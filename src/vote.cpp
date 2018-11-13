// Copyright (c) 2018 Project Orchid
// Copyright (c) 2018 The Pinkcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "votedb.h"
#include "vote.h"
#include "init.h"
#include "util.h"
#include "zlib.h"
#include "script.h"
#include <random>
#include <string>
#include <iostream>
#include <sstream>



CVote::CVote()
{
    clear();
}

ActivePoll::ActivePoll()  :  pIt(*(new PollStack::iterator)), bIt(*(new BallotStack::iterator))
{
    ID = 0; BID =0;

    CVotePoll *dummyPoll = new CVotePoll();
    CVoteBallot *dummyBallot = new CVoteBallot();
    PollStack *dummyPollStack = new PollStack();
    BallotStack *dummyBallotStack = new BallotStack();

    dummyPollStack->insert(make_pair((CPollID)0, *dummyPoll));
    dummyBallotStack->insert(make_pair((CPollID)0, *dummyBallot));

    setActive(dummyPollStack->find(0), dummyBallotStack->find(0), SET_CLEAR); poll->clear();
}
bool ActivePoll::isLocal()
{
    return (vIndex->pollCache.find(poll->ID) == vIndex->pollCache.end());
}

void CVotePoll::pollCopy(const CVotePoll& poll)
{
    clear();

    ID = poll.ID;
    Name = poll.Name;
    Question = poll.Question;
    Start = poll.Start;
    End = poll.End;
    Flags = poll.Flags;
    OpCount = poll.OpCount;
    strAddress = poll.strAddress;

    assert(poll.Option.size() == poll.nTally.size()); // Vectorize them together later.

    vector<CVoteTally>::const_iterator ti = poll.nTally.begin();
    for (vector<CPollOption>::const_iterator i = poll.Option.begin(); i != poll.Option.end() ; i++)
    {

        Option.push_back(*i);
        nTally.push_back(*ti);
        ti++;
    }

    nHeight = poll.nHeight;
    hash = poll.hash;
}

bool CVotePoll::isValid()
{
    return true; // for now.
}

bool CVotePoll::isActive()
{
    {
    TRY_LOCK(cs_main, lockMain);

    CPollTime pbNow = GetPollTime2(pindexBest->nTime);

    return (Start < pbNow && pbNow < End);
    }
}

bool CVotePoll::haveParent()
{
    if (Option.size() < 1U)
        return false;

    vector<CPollOption>::iterator pIt = Option.begin();
    CPollID ParentID = stoi(*pIt);

    return (ParentID != 0 && vIndex->pollCache.find(ParentID) != vIndex->pollCache.end());

}
bool CVotePoll::parentEnded()
{
    if (Option.size() < 1U)
        return false;

    vector<CPollOption>::iterator pIt = Option.begin();
    CPollID ParentID = stoi(*pIt);

    return (ParentID != 0 && vIndex->pollCache.at(ParentID).hasEnded());

}

bool CVotePoll::hasEnded()
{
    return (GetPollTime(End, nHeight) < pindexBest->nTime && vIndex->pollCache.find(ID) != vIndex->pollCache.end());
}

bool CVotePoll::isComplete()
{
    if (Name.size() < 2U || Question.size() < 6U || End <= Start ||
        ID == 0 || strAddress.size() < 34U)
        return false;

    if (Option.size() > 1U && nHeight > 0 && hash > 0)
        return true;
    if (isFund() && nHeight > 0 && hash > 0)
        return true;

    if (isFund() && Option.size() > 0U && isLocal())
        return true;
    if (Option.size() > 1U && isLocal())
        return true;

    return false;
}

bool CVotePoll::isMine()
{
    // Do I have the private key for it?
    CKeyID pollKey;
    if (CBitcoinAddress(strAddress).GetKeyID(pollKey))
        return (pwalletMain->HaveKey(pollKey));


    // If it's unset then it must be local and therefore mine... but should check anyway. It's cheap.
    return isLocal();
}

bool CVotePoll::isLocal()
{
    // If we're not in the Cache and Our ID is set then we MUST be local.
    return (ID != 0 && vIndex->pollCache.find(ID) == vIndex->pollCache.end());
}

bool CVotePoll::isConsensus()
{
    vector<CPollOption>::const_iterator pOpt = Option.begin();

    if(pOpt == Option.end() || Option.size() != 3U || nTally.size() != 3U)
        return false;

    pOpt++; // Option 1 is overloaded to hold the Parent PollID if it's a claim poll;
    if(string(pOpt->data()) != "Approve.")
        return false;
    pOpt++;
    if(string(pOpt->data()) != "Disapprove.")
        return false;

    // POLL_CLAIM covers POS/FPOS/POW for us.
    if(Flags & (CVote::POLL_CLAIM + CVote::POLL_ALLOW_D4L))
        return true;
    else if (Flags == CVote::POLL_ENFORCE_POS)
        return true;

    return false;
}

bool CVotePoll::isApproved()
{
    // 2/3rds approval required to approve a consensus change or claim on a fundraiser/bounty.
    return (getConsensus() > (POLL_CONSENSUS_PRECISION * 2 / 3));
}
bool CVotePoll::isFullyApproved()
{
    // 90% approval required to override funding destination of parent poll.
    // May also be used to manage other sensitive decisions.
    return (getConsensus() > (POLL_CONSENSUS_PRECISION * 90 / 100));
}

uint64_t CVotePoll::getConsensus()
{
    if(!isConsensus() || !isComplete())
        return 0;

    int typeCount = 0;
    uint64_t pollSuccess = 0;

    if(acceptPOS())
    {
        uint64_t nTotal = (nTally[1].POS + nTally[2].POS) * POLL_CONSENSUS_PRECISION;
        uint64_t nApproved = (nTally[1].POS) * POLL_CONSENSUS_PRECISION;
        if (nTotal > 0)
            pollSuccess += nApproved / nTotal;
        else
            pollSuccess += POLL_CONSENSUS_PRECISION/2;
        typeCount++;
    }
    if(acceptFPOS())
    {
        uint64_t nTotal = (nTally[1].FPOS + nTally[2].FPOS) * POLL_CONSENSUS_PRECISION;
        uint64_t nApproved = (nTally[1].FPOS) * POLL_CONSENSUS_PRECISION;
        if (nTotal > 0)
            pollSuccess += nApproved / nTotal;
        else
            pollSuccess += POLL_CONSENSUS_PRECISION/2;
        typeCount++;
    }
    if(acceptPOW())
    {
        uint64_t nTotal = (nTally[1].POW + nTally[2].POW) * POLL_CONSENSUS_PRECISION;
        uint64_t nApproved = (nTally[1].POW) * POLL_CONSENSUS_PRECISION;
        if (nTotal > 0)
            pollSuccess += nApproved / nTotal;
        else
            pollSuccess += POLL_CONSENSUS_PRECISION/2;
        typeCount++;
    }
    if(acceptPOS())
    {
        uint64_t nTotal = (nTally[1].D4L + nTally[2].D4L) * POLL_CONSENSUS_PRECISION;
        uint64_t nApproved = (nTally[1].D4L) * POLL_CONSENSUS_PRECISION;
        if (nTotal > 0)
            pollSuccess += nApproved / nTotal;
        else
            pollSuccess += POLL_CONSENSUS_PRECISION/2;
        typeCount++;
    }

    return (pollSuccess / typeCount);
}

bool CVotePoll::onlyPOS() { return (Flags == CVote::POLL_ENFORCE_POS); }
bool CVotePoll::acceptPOS() { return (Flags & CVote::POLL_ALLOW_POS || Flags == CVote::POLL_ENFORCE_POS); }
bool CVotePoll::acceptFPOS() { return (Flags & CVote::POLL_ALLOW_FPOS); }
bool CVotePoll::acceptPOW() { return (Flags & CVote::POLL_ALLOW_POW); }
bool CVotePoll::acceptD4L() { return (Flags & CVote::POLL_ALLOW_D4L); }
bool CVotePoll::isFund() { return (Flags & CVote::POLL_FUNDRAISER); }
bool CVotePoll::isBounty() { return (Flags == CVote::POLL_BOUNTY); }
bool CVotePoll::isP2POLL() { return (Flags & CVote::POLL_PAY_TO_POLL);}
bool CVotePoll::isClaim() { return Flags == CVote::POLL_CLAIM; }

void ActivePoll::setActive(const PollStack::iterator &pit, const BallotStack::iterator &bit, unsigned int flags)
{
     if (flags & SET_POLL)
     {
         ID = pit->first;
         poll = &pit->second;
         pIt = pit;
     }
     if (flags & SET_BALLOT)
     {
        BID = bit->first;
        ballot = &bit->second;
        bIt = bit;
     }
     if (flags == SET_CLEAR)
     {
         BID = ID = 0;
         bIt = bit;
         pIt = pit;
         poll =  &pit->second;
         ballot = &bit->second;
     }
}

void CVote::clear()
{
    CVotePoll *activePoll = new CVotePoll();
    CVoteBallot *activeBallot = new CVoteBallot();

    pollCache.clear();
    pollStack.clear();
    ballotStack.clear();

    pollStack.insert(make_pair(activePoll->ID, *activePoll));
    pollCache.insert(make_pair(activePoll->ID, *activePoll));
    ballotStack.insert(make_pair(activeBallot->PollID, *activeBallot));

    current.poll = activePoll;
    current.ballot = activeBallot;

    current.setActive(current.pIt, current.bIt, ActivePoll::SET_CLEAR);

    this->readyPoll = false;
    this->optionCount = 0;
    this->ballotCount = 0;
}

bool CVote::newPoll(CVotePoll* poll)
{
    if (poll->ID == 0)
        while (pollStack.find(poll->ID) != pollStack.end() ||
               pollCache.find(poll->ID) != pollCache.end() ||
               poll->ID == 0)
        {
            poll->ID = getNewPollID();
        }
    else
    {
        printf("[NewPoll]: Poll already has ID set. \n");
        return false;
    }

    if (ballotStack.find(poll->ID) != ballotStack.end())
    {
        LOCK(vIndex->cs_wallet);
        // We have a random ballot that no poll exists for.
        CVoteDB(vIndex->strWalletFile).EraseBallot(ballotStack.at(poll->ID));
        ballotStack.erase(poll->ID);
    }

    CVoteBallot* ballot = new CVoteBallot();
    ballot->PollID = poll->ID; ballot->OpSelection = 0;

    pollStack.insert(make_pair(poll->ID, *poll));
    ballotStack.insert(make_pair(ballot->PollID, *ballot));

    current.setActive(pollStack.find(poll->ID), ballotStack.find(ballot->PollID));

    readyPoll = false;
    return true;
}


bool CVote::getPoll(CPollID& pollID, CVotePoll* poll)
{
    // Save our current ballot and poll
    // saveActive();

    poll->OpCount = (COptionID)poll->Option.size(); // Do something harmless with poll to get rid of the compiler warning.

    if (pollStack.find(pollID) != pollStack.end())
    {
        poll = &pollStack.at(pollID);
        return true;
    } else if (pollCache.find(pollID) != pollCache.end())
    {
        poll = &pollCache.at(pollID);
        return true;
    }
    return false;
}


// Because c++ doesn't like comparing structures/classes. Will define == operator later.
bool pollCompare(CVotePoll* a, CVotePoll* b)
{
    if (a->ID != b->ID || a->Name != b->Name ||
        a->Question != b->Question || a->Start != b->Start ||
        a->End != b->End || a->Flags != b->Flags || a->hash != b->hash ||
        a->nHeight != b->nHeight)
    return false;

    if (a->Option.size() != b->Option.size() || a->Option.size() != b->nTally.size())
            return false;

    for (unsigned int i=0 ; i < a->Option.size() ; i++)
    {
        if (a->Option[i] != b->Option[i] || a->nTally[i].POS != b->nTally[i].POS ||
            a->nTally[i].FPOS != b->nTally[i].FPOS || a->nTally[i].POW != b->nTally[i].POW ||
            a->nTally[i].D4L != b->nTally[i].D4L)
            return false;
    }
    return true;
}

CPollID CVote::getNewPollID()
{

    CPollID randomID = rGen32();
    return randomID;
}

bool CVote::setPoll(CPollID& pollID)
{
    bool setPoll = false;
    if (pollCache.find(pollID) != pollCache.end() && pollStack.find(pollID) == pollStack.end())
    {
        CVotePoll *newPoll = new CVotePoll();
        newPoll->pollCopy(pollCache.find(pollID)->second);
        pollStack.insert(make_pair(pollID, *newPoll));
        readyPoll = false;
        setPoll=true;
    }
    if (pollStack.find(pollID) != pollStack.end())
    {
        current.setActive(pollStack.find(pollID), current.bIt, ActivePoll::SET_POLL);
        readyPoll = false;
        setPoll=true;
    }

    if (setPoll && ballotStack.find(pollID) != ballotStack.end())
    {
        current.setActive(current.pIt, ballotStack.find(pollID), ActivePoll::SET_BALLOT);
    } else if (setPoll) {
        CVoteBallot *newBallot = new CVoteBallot;
        newBallot->PollID = pollID;
        newBallot->OpSelection = 0;
        ballotStack.insert(make_pair(newBallot->PollID, *newBallot));
        current.setActive(current.pIt, ballotStack.find(newBallot->PollID), ActivePoll::SET_BALLOT);
    }

    return setPoll;
}

CVotePoll CVote::getActivePoll()
{
    return *current.poll;
}

bool CVote::validatePoll(const CVotePoll* poll, const bool& fromBlockchain)
{
    CVotePoll *p = new CVotePoll();
    p->pollCopy(*poll);

    readyPoll = true;  // I wrote this weird, I know. I'll fix it all later.

    // If it is from the blockchain and therefore not Local,
    // it will still return true if we don't have it at all.
    // I'll rewrite isLocal as a wrapper around 'haveCached()'
    // function or something later. Yes I could have done that
    // more quickly than I spent writing this comment, but then
    // I would be denying you the entertainment
    if (!p->isLocal())
        readyPoll = false;

    if (p->Start >= p->End)
        readyPoll = false;
    if (p->strAddress.size() < 1U)
        readyPoll = false;
    if (p->Question.size() < 1U)
        readyPoll = false;
    if (p->Name.size() < 1U)
        readyPoll = false;
    if (p->Option.size() < 1U)
        readyPoll = false;
    if (p->Start < (GetPollTime2(pindexBest->nTime) + 1))
        readyPoll = false;

    if (p->isClaim() && !fTestNet)
    {
        if(p->Start < GetPollTime2(GetAdjustedTime() + (24 * 7))) { readyPoll = false; }
        else if (p->Start < p->End + (24 * 14)) { readyPoll = false; }
        else if (p->haveParent() && p->parentEnded()) { readyPoll = false; }
    }

    if (fromBlockchain)
        readyPoll = p->isValid();

    delete p;

    return readyPoll;
}

bool CVote::commitPoll(const CVotePoll *poll, string& hash, const bool &fromBlockchain)
{
    string fromAddress = ""; // Not yet supported.
    bool ret = false;
    bool isLocal = fromBlockchain; // So WriteVote is clearer, because the default write IS local.
    {
        LOCK(vIndex->cs_wallet);
        if (this->pollCache.find(poll->ID) == this->pollCache.end() && fromBlockchain)
            ret = CVoteDB(this->strWalletFile).WriteVote(*poll, !isLocal);

        if (ret)
            this->pollCache.insert(make_pair(poll->ID, *poll));

        if (ret && pollStack.find(poll->ID) != pollStack.end() && !pollCompare(&pollCache.at(poll->ID), &pollStack.at(poll->ID)))
        {
            CPollID newID = poll->ID;
            while (pollCache.find(newID) != pollCache.end())
            {
                newID = getNewPollID();
            }
            CVoteDB(this->strWalletFile).EraseVote(pollStack.at(poll->ID));

            pollStack.at(newID).ID = newID;
            CVoteDB(this->strWalletFile).WriteVote(pollStack.at(newID));

        }

    }
    if (!fromBlockchain)
        return commitToChain(poll, fromAddress, hash);

    return ret;
}

bool CVote::havePollReady()
{
    return readyPoll;
}

bool CVote::commitToChain(string& hash)
{
    return commitToChain(vIndex->current.poll, "", hash);
}

void CVote::setReady(const bool isReady)
{
    readyPoll = isReady;
}

bool CVote::commitToChain(const CVotePoll* poll, const string& fromAddress, string& hash)
{
    CWalletTx wtx;
    wtx.isPoll = true;

    int64_t addressBalance = 0;
    map<string, CBitcoinAddress> mapAccountAddresses;
    if (fromAddress != "")
    {
        CKeyID fromKey;
        if (CBitcoinAddress(fromAddress).GetKeyID(fromKey) && pwalletMain->HaveKey(fromKey)) // This address belongs to me
        {
            //CTxDestination fromDest = fromKey;
            addressBalance = pwalletMain->GetAddressBalance(fromKey);
        }
        else
            return false;
    }

    if (pwalletMain->IsLocked() || fWalletUnlockStakingOnly)
        return false;


    // Note: From address DOES NOT WORK. Neither does FromAccount in rpc sendfrom for that matter. Go figure.
    // I'll fix that later now that I know what the problem is (which is a bit of a mess, frankly. Accounting
    // is terribly incomplete. No wonder it was depreciated. It's author never completed it as it is being used
    // and it appears that it was ultimately replaced by coincontrol.)

    if ((addressBalance > 101 * COIN && fromAddress != "") ||
            (pwalletMain->GetBalance() < 101 * COIN && fromAddress == ""))
        return false; // need at least 100 coins + txfee for poll.

    vector<unsigned char>rawPoll;
    getRawPoll(rawPoll, poll);

    CScript scriptPoll = (CScript() << OP_VOTE) <= rawPoll;

    string sNarr = "";
    // Send
    string strError = pwalletMain->SubmitPollTx(scriptPoll, wtx);
    if (strError != "")
        printf("[commitToChain]: Poll Commit Failed. Reason: %s \n", strError.c_str());

    hash.clear();
    hash = wtx.GetHash().GetHex();
    return (bool)hash.size();
    //return false; // we're getting there, but not yet.
}

int64_t GetPollTime(const CPollTime &pTime, const int &blockHeight)
{
    int bHeight = blockHeight;
    /* If we're within 10k blocks of our 4 BLOCKCOUNT years cutoff,
     * interpret the time based on pTime value restrictions.
     * This is important since we can't be sure when the poll will
     * first be accepted into a block when it is created and submitted
     * to the mempool. 10k blocks gives about a week of padding.     */
    if (bHeight - 10000 < (YEARLY_BLOCKCOUNT * 4) && bHeight + 10000 > (YEARLY_BLOCKCOUNT * 4))
    {
        if(pTime <= 2400) // 100 days past baseTime.
            bHeight = bHeight + 10000;   // pTime is set too far behind to be pre-YB*4 at this blockheight.
        else
            bHeight = bHeight - 10000;   // pTime is set too far ahead to be post-YB*4 at this blockheight.
    }


    /* Reset PollTime every 4 BLOCKCOUNT years.
     * CPollTime is incremented every hour since the first hour
     * after GenesisBlock So it can contain 7 1/2 years of time. */
    int baseClock = bHeight / (YEARLY_BLOCKCOUNT * 4);
    int baseHeight = baseClock * (YEARLY_BLOCKCOUNT * 4);

    CBlockIndex *tIndex = FindBlockByHeight(baseHeight);

    int64_t baseTime = tIndex->nTime / 60 / 60;
    int64_t pollTime = (baseTime + (int64_t)pTime) * 60 * 60;

    return pollTime;
}

CPollTime GetPollTime2(const int64_t &uTime, const int &blockHeight)
{

    int bHeight = blockHeight;

    /* Reset PollTime every 4 BLOCKCOUNT years.
     * CPollTime is incremented every hour since the first hour
     * after GenesisBlock So it can contain 7 1/2 years of time. */
    int baseClock = bHeight / (YEARLY_BLOCKCOUNT * 4);
    bHeight = baseClock * (YEARLY_BLOCKCOUNT * 4);

    CBlockIndex *tIndex = FindBlockByHeight(bHeight);
    int64_t baseTime = tIndex->nTime / 60 / 60;
    int64_t pTime = uTime / 60 / 60;
    pTime = pTime - baseTime;

    CPollTime pollTime;
    if (pTime > 0)
        pollTime = pTime;
    else
        pollTime = 0;

    return pollTime;
}

bool GetPollHeight(CPollID& pollID, int& pollHeight)
{
    CVote t;
    if(!t.setPoll(pollID))
        return false;

    CVotePoll temp = t.getActivePoll();
    if (temp.nHeight != 0)
        pollHeight = temp.nHeight;
    else
        pollHeight = 0;

    return true;
}

VDBErrors CVote::LoadVoteDB(bool& fFirstRunRet)
{
    if (!fFileBacked)
        return VDB_LOAD_OK;
    fFirstRunRet = false;

    {
    LOCK(vIndex->cs_wallet);

    VDBErrors nLoadVoteDBRet = CVoteDB(strWalletFile,"cr+").LoadVote(this);

    if (nLoadVoteDBRet != VDB_LOAD_OK)
        return nLoadVoteDBRet;
    }

    NewThread(ThreadFlushVoteDB, &strWalletFile);
    return VDB_LOAD_OK;
}

bool processRawPoll(const vector<unsigned char>& rawPoll, const uint256& hash, const int& nHeight, const bool &checkOnly, const bool &fromBlockchain)
{
    
    // Let's catch this one early.
    // POLL_HEADER + SIZEOF(COMPRESSED_NEWPOLL) + Z_HEADER + Z_ADLER32
    if (rawPoll.size() < POLL_HEADER_SIZE + 5U + 2U + 4U) { printf("[processRawPoll]: Illegal RawPoll.\n"); return false; }
    if (rawPoll[POLL_HEADER_SIZE] != 0x78) { printf("[processRawPoll]: Illegal Z_CMF."); return false; }
    if (rawPoll[POLL_HEADER_SIZE+1] != 0xda) { printf("[processRawPoll]: Rejecting unofficial Z_FLG.\n"); return false; }

    // RawPoll Construction - POLL_HEADER + POLLID + FLAGS + START + END + SIZE_OF_COMPRESSED_DATA

    // Check that we have a good poll.
    vector<unsigned char>::const_iterator rIt = rawPoll.begin();
    uint8_t opCount = ((*rIt >> 4) & (uint8_t)7); // 0b00000111

    // Grab our Big Endian and convert it back for dataSize.
    unsigned char b[2] { 0, 0 };
    memcpy(&b[0], &*rIt, 2); b[0] &= 0x0F;

    uint16_t dataSize;
    dataSize = (b[0] << 8) + b[1];

    const size_t pollSize = POLL_HEADER_SIZE + dataSize;
    if (pollSize > MAX_VOTE_SIZE) { printf("[processRawPoll: Poll Exceeds limit. \n"); return false; }
    if (rawPoll.size() != pollSize) { printf("[processRawPoll: Poll has invalid size.\n"); return false; }

    rIt += POLL_INFO_SIZE;

    CPollID pollID;
    memcpy(&pollID, &*rIt, POLL_ID_SIZE);
    rIt += POLL_ID_SIZE;

    if (vIndex && vIndex->pollCache.find(pollID) != vIndex->pollCache.end() && !(nBestHeight < GetNumBlocksOfPeers()))
        return false;

    if (!checkOnly)
    {
        try {
            bool success = false;
            CVotePoll *checkPoll = new CVotePoll();

            checkPoll->ID = pollID;

            memcpy(&checkPoll->Flags, &*rIt, POLL_FLAG_SIZE); rIt += POLL_FLAG_SIZE;
            memcpy(&checkPoll->Start, &*rIt, POLL_TIME_SIZE); rIt += POLL_TIME_SIZE;
            memcpy(&checkPoll->End, &*rIt, POLL_TIME_SIZE ); rIt += POLL_TIME_SIZE;

            vector<unsigned char> dvchPoll;
            vector<unsigned char> cvchPoll(&*rawPoll.begin() + POLL_HEADER_SIZE, &*rawPoll.begin() + POLL_HEADER_SIZE + dataSize);

            charZip(cvchPoll, dvchPoll, true);

            // Check our adler32 checksum.
            char bEnd[4]; memcpy(&bEnd, &*rawPoll.end()-4, 4); // adler32 is in BigEndian on our compressed file.
            char lEnd[4] = {bEnd[3], bEnd[2], bEnd[1], bEnd[0]};
            uint32_t *dAdler = reinterpret_cast <uint32_t*>(lEnd);

            uint32_t vAdler = adler32(0L, Z_NULL, 0);
            vAdler = adler32(vAdler, (Bytef *)dvchPoll.data(), dvchPoll.size());

            if (vAdler != *dAdler) { printf("[processRawPoll]: Adler32 check failed.\n"); return false;}

            vector<unsigned char>::const_iterator cIt = dvchPoll.begin();

            if (dvchPoll.size() != POLL_NAME_SIZE + POLL_QUESTION_SIZE + (opCount * POLL_OPTION_SIZE) + POLL_ADDRESS_SIZE)
            { printf("[Process RawPoll]: Uncompressed does not match initially reported size.\n"); return false; }

            char charName[POLL_NAME_SIZE];
            memcpy(&charName, &*cIt, POLL_NAME_SIZE);
            checkPoll->Name = charName;
            cIt += POLL_NAME_SIZE;

            char charQuestion[POLL_QUESTION_SIZE];
            memcpy(&charQuestion, &*cIt, POLL_QUESTION_SIZE);
            checkPoll->Question = charQuestion;
            cIt += POLL_QUESTION_SIZE;

            for ( uint8_t i = 0 ; i < opCount ; i++ )
            {
                char pOpt[POLL_OPTION_SIZE];
                memcpy(&pOpt, &*cIt, POLL_OPTION_SIZE);
                checkPoll->Option.push_back(string(pOpt));
                cIt += POLL_OPTION_SIZE;
            }

            checkPoll->nTally.resize(checkPoll->Option.size());

            char charAddress[POLL_ADDRESS_SIZE];
            memcpy(&charAddress, &*cIt, POLL_ADDRESS_SIZE);
            checkPoll->strAddress = charAddress;

            checkPoll->OpCount = checkPoll->Option.size();
            checkPoll->nHeight = nHeight;
            checkPoll->hash = hash;

            string strHash = "";
            success = vIndex->validatePoll(checkPoll, fromBlockchain);
            if (success && fromBlockchain)
                vIndex->commitPoll(checkPoll, strHash, fromBlockchain);

            printf("Success. %u %s %s %u %u %s. \n", checkPoll->ID, checkPoll->Name.c_str(), checkPoll->Question.c_str(), checkPoll->Start, checkPoll->End, checkPoll->hash.GetHex().c_str());

            return success;
        } catch (...) {
            printf("[processRawPoll]: Failed to process raw poll. Debug or contact your friendly neighborhood Dev for assistance./n");
            return false;
        }
    }

    return true;
}

bool getRawPoll(vector<unsigned char>& rawPoll, const CVotePoll* inPoll)
{
    CVotePoll ourPoll;

    ourPoll.pollCopy(*inPoll);

    ourPoll.Name.resize(POLL_NAME_SIZE);
    ourPoll.Question.resize(POLL_QUESTION_SIZE);
    for (uint8_t i = 0; i < ourPoll.OpCount; i++)
        ourPoll.Option[i].resize(POLL_OPTION_SIZE);
    ourPoll.strAddress.resize(POLL_ADDRESS_SIZE);

    rawPoll.resize(MAX_VOTE_SIZE);
    vector<unsigned char>::iterator rIt = rawPoll.begin();

    // rIt->n = (vIndex->current.poll->OpCount << 4);

    rIt += POLL_INFO_SIZE;

    memcpy(&*rIt, &ourPoll.ID, POLL_ID_SIZE); rIt += POLL_ID_SIZE;
    memcpy(&*rIt, &ourPoll.Flags, POLL_FLAG_SIZE); rIt += POLL_FLAG_SIZE;
    memcpy(&*rIt, &ourPoll.Start, POLL_TIME_SIZE); rIt += POLL_TIME_SIZE;
    memcpy(&*rIt, &ourPoll.End, POLL_TIME_SIZE); rIt += POLL_TIME_SIZE;

    vector<unsigned char>crushRaw;
    crushRaw.resize(POLL_NAME_SIZE + POLL_QUESTION_SIZE + (ourPoll.OpCount * POLL_OPTION_SIZE) + POLL_ADDRESS_SIZE);
    vector<unsigned char>::iterator cIt = crushRaw.begin();

    memcpy(&*cIt, &*ourPoll.Name.c_str(), POLL_NAME_SIZE); cIt += POLL_NAME_SIZE;
    memcpy(&*cIt, &*ourPoll.Question.c_str(), POLL_QUESTION_SIZE); cIt += POLL_QUESTION_SIZE;

    for (vector<CPollOption>::const_iterator it = ourPoll.Option.begin(); it < ourPoll.Option.end(); it++)
    { memcpy(&*cIt, &*it->c_str(), POLL_OPTION_SIZE); cIt += POLL_OPTION_SIZE; }

    memcpy(&*cIt, &*ourPoll.strAddress.c_str(), POLL_ADDRESS_SIZE);

    vector<unsigned char> crush(&crushRaw[0], &crushRaw[0] + crushRaw.size());
    vector<unsigned char> boom;
    vector<unsigned char> check;

    charZip(crush, check);
    charZip(check, boom, true);

    bool zSuccess = (crush==boom);

    if (zSuccess)
    {
        rIt -= POLL_HEADER_SIZE;
        int16_t compressedSize = (int16_t)check.size();

        // We Want Big Endian here because we'll be overloading b[0] with a flag and our opcount.
        char b[2]; b[0] = compressedSize >> 8; b[1] = (compressedSize & 0x00FF);

        memcpy(&*rIt, &b[0], 2);
        *rIt |= (1U << 7 );  //We are a VotePoll.
        *rIt |= (ourPoll.OpCount << 4);
        rIt += POLL_HEADER_SIZE;
        memcpy(&*rIt, &*check.data(), check.size());

        // vector<CRawPoll> temp(&rawPoll.begin(), &rawPoll.begin() + compressedSize + POLL_HEADER_SIZE);
        rawPoll.resize(compressedSize + POLL_HEADER_SIZE);
        // rawPoll = temp;
        return true;
    } else {
        printf("[getRawPoll]: Failed to compress poll. \n");
        return false;
    }
}

// I don't think we need this function. Leaving for the moment.
bool processRawBallots(const vector<unsigned char>& rawBallots, const BLOCK_PROOF_TYPE &t, const bool &undo, const bool &checkOnly)
{
    if (checkOnly)
        return true; // todo, should we ever need this.

    BallotStack ballots;
    ballots.clear();
    if (getBallots(rawBallots, ballots))
    {
        tallyBallots(ballots, t, undo);
        return true;
    }

    return false;
}

void erasePoll(const uint256 &hash)
{
    // best way to do it until I decide to make a txid->pollID map.
    for (PollStack::iterator it = vIndex->pollCache.begin(); it != vIndex->pollCache.end(); it++)
        if (it->second.hash == hash)
        {
            LOCK(vIndex->cs_wallet);
            PollStack::iterator pIt = vIndex->pollStack.find(it->second.ID);
            BallotStack::iterator bIt = vIndex->ballotStack.find(it->second.ID);

            if (pIt != vIndex->pollStack.end()){
                CVoteDB(vIndex->strWalletFile).EraseVote(pIt->second);
                vIndex->pollStack.erase(pIt);
            }
            if (bIt != vIndex->ballotStack.end()) {
                CVoteDB(vIndex->strWalletFile).EraseBallot(bIt->second);
                vIndex->ballotStack.erase(bIt);
            }
            CVoteDB(vIndex->strWalletFile).EraseVote(it->second);
            vIndex->pollCache.erase(it);
        }
}
void erasePoll(const CPollID& ID)
{
    PollStack::iterator it = vIndex->pollCache.find(ID);
    PollStack::iterator pIt = vIndex->pollStack.find(ID);
    BallotStack::iterator bIt = vIndex->ballotStack.find(ID);

    {
        LOCK(vIndex->cs_wallet);
        if (pIt != vIndex->pollStack.end()){
            CVoteDB(vIndex->strWalletFile).EraseVote(pIt->second);
            vIndex->pollStack.erase(pIt);
        }
        if (bIt != vIndex->ballotStack.end()) {
            CVoteDB(vIndex->strWalletFile).EraseBallot(bIt->second);
            vIndex->ballotStack.erase(bIt);
        }
        if (it != vIndex->pollCache.end()) {
            CVoteDB(vIndex->strWalletFile).EraseVote(it->second);
            vIndex->pollCache.erase(it);
        }
    }
}
bool isLocal()
{
    return (vIndex->pollCache.find(vIndex->current.poll->ID) == vIndex->pollCache.end());
}

bool selectBallots(vector<unsigned char> &vchBallots, const BLOCK_PROOF_TYPE t, const uint64_t startFrom)
{
    if (vIndex->ballotStack.size() < startFrom + 1U)
        return false;

    PollStack *p = &vIndex->pollCache;
    unsigned char ballotPair = '\0';
    vector<unsigned char> IDPair;
    bool first = true;
    uint8_t count = 0;

    map<BLOCK_PROOF_TYPE, CVote::flags> mFlags;
    mFlags.insert(make_pair(BLOCK_PROOF_POS, CVote::POLL_ALLOW_POS));
    mFlags.insert(make_pair(BLOCK_PROOF_FPOS, CVote::POLL_ALLOW_FPOS));
    mFlags.insert(make_pair(BLOCK_PROOF_POW, CVote::POLL_ALLOW_POW));

    for (BallotStack::const_iterator bIt = next(vIndex->ballotStack.begin(), startFrom); bIt != vIndex->ballotStack.end(); bIt++)
    {
        CPollIDDest vchID(bIt->first);
        const unsigned char &vchVote = bIt->second.OpSelection;

        if (vchVote != 0 && p->find(vchID.ID) != p->end() && (p->at(vchID.ID).Flags & mFlags.at(t)) &&
                count < 100 && p->at(vchID.ID).isActive())
        {
            IDPair.insert(IDPair.end(), vchID.begin(), vchID.end());
            ballotPair |= first ? vchVote : (vchVote << 4);

            if (!first)
            {
                vchBallots.insert(vchBallots.end(), IDPair.begin(), IDPair.end());
                vchBallots.insert(vchBallots.end(), ballotPair);
                IDPair.clear();
                ballotPair = '\0';
            }
            first = !first;
            count++;
        }
    }
    if (!first)
    {
        vchBallots.insert(vchBallots.end(), IDPair.begin(), IDPair.end());
        vchBallots.insert(vchBallots.end(), ballotPair);
    }

    if (count > 0)
        vchBallots.insert(vchBallots.begin(), count);
    return (count > 0);
}

bool checkBallots(const vector<unsigned char> &vchBallots)
{
    int8_t szData = vchBallots[0];
    if (szData < 101 && szData > 0)
    {
        if (szData & 1U)
            szData = ((szData -1) / 2 * 5) + 5;
        else
            szData = (szData -1) / 2 * 5;

        return (vchBallots.size() == szData + 1U);
    }
    return false;
}
bool getBallots(const vector<unsigned char> &vchBallots, BallotStack &stackBallots)
{
    CVoteBallot ballot;
    CPollIDDest holder;
    uint8_t count = 0;
    uint8_t checkCount = 0;
    vector<unsigned char> workBallots(vchBallots.begin(), vchBallots.end());

    if (vchBallots.size() > 5U)
    {
        checkCount = *workBallots.begin();
        workBallots.erase(workBallots.begin());
    } else {
        return false;
    }

    // Simple, fast.
    while (true)
    {
        if (workBallots.size() > 8U)
        {
            holder = vector<unsigned char>(workBallots.begin(), workBallots.begin()+4);
            ballot.OpSelection = (*(workBallots.begin()+8) & 0x0F);
            ballot.PollID = holder.ID;
            if (stackBallots.find(ballot.PollID) == stackBallots.end())
            {
                stackBallots.insert(make_pair(ballot.PollID, ballot));
            }

            holder = vector<unsigned char>(workBallots.begin()+4, workBallots.begin()+8);
            ballot.OpSelection = (*(workBallots.begin()+8) >> 4);
            ballot.PollID = holder.ID;
            if (stackBallots.find(ballot.PollID) == stackBallots.end())
            {
                stackBallots.insert(make_pair(ballot.PollID, ballot));
            }

            count += 2;
            workBallots.erase(workBallots.begin(), workBallots.begin()+9);
        } else if (workBallots.size() == 5U)
        {
            holder = vector<unsigned char>(workBallots.begin(), workBallots.begin()+4);
            ballot.OpSelection = (*(workBallots.begin()+4) & 0x0F);
            ballot.PollID = holder.ID;
            if (stackBallots.find(ballot.PollID) == stackBallots.end())
            {
                stackBallots.insert(make_pair(ballot.PollID, ballot));
            }

            count++;
            workBallots.erase(workBallots.begin(), workBallots.begin()+5);
        } else if (workBallots.size() == 0U) {
            if (count == checkCount)
                return true;
            else
                return false; // Number of ballots doesn't match count.
        } else {
            return false; // Incorrect number of elements. Unable to be sure we have accurate ballots.
        }
    }

}
bool verifyBallots(const BallotStack &stackBallots, const BLOCK_PROOF_TYPE &t, vector<BallotStack::const_iterator> &badIt)
{
    bool allGood = true;

    map<BLOCK_PROOF_TYPE, CVote::flags> mFlags;
    mFlags.insert(make_pair(BLOCK_PROOF_POS, CVote::POLL_ALLOW_POS));
    mFlags.insert(make_pair(BLOCK_PROOF_FPOS, CVote::POLL_ALLOW_FPOS));
    mFlags.insert(make_pair(BLOCK_PROOF_POW, CVote::POLL_ALLOW_POW));

    for (BallotStack::const_iterator bIt = stackBallots.begin(); bIt != stackBallots.end(); bIt++)
    {
        PollStack::iterator pIt = vIndex->pollCache.find(bIt->first);
        if (pIt == vIndex->pollCache.end() || !(pIt->second.Flags & mFlags.at(t)) ||
                 bIt->second.OpSelection == 0 || bIt->second.OpSelection > pIt->second.nTally.size())
        {
            badIt.push_back(bIt);
            allGood = false;
        }
    }
    return allGood;

}
void tallyBallots(const BallotStack &stackBallots, const BLOCK_PROOF_TYPE &t, const bool &undo)
{
    vector<BallotStack::const_iterator> badIt;
    BallotStack workStack = stackBallots;
    if (!verifyBallots(stackBallots, t, badIt))
    {
        for (vector<BallotStack::const_iterator>::const_iterator remIt = badIt.begin(); remIt != badIt.end(); remIt++)
            workStack.erase(*remIt);
        printf("Removed %" PRIszu " bad ballots.\n", badIt.size());
    }

    {
        LOCK(vIndex->cs_wallet);
        for (BallotStack::const_iterator bIt = stackBallots.begin(); bIt != stackBallots.end(); bIt++)
        {
            const CVoteBallot &ballotVote = bIt->second;
            vector<CVoteTally>::iterator onPoll = (vIndex->pollCache.at(bIt->first).nTally.begin()+(ballotVote.OpSelection-1));

            if (t == BLOCK_PROOF_POS)
            {
                onPoll->POS = undo ? onPoll->POS - 1 : onPoll->POS + 1;
            } else if (t == BLOCK_PROOF_FPOS)
            {
                onPoll->FPOS = undo ? onPoll->FPOS - 1 : onPoll->FPOS + 1;
            } else if (t == BLOCK_PROOF_POW)
            {
                onPoll->POW = undo ? onPoll->POW - 1 : onPoll->POW + 1;
            }

            CVoteDB(vIndex->strWalletFile).WriteVote(vIndex->pollCache.at(bIt->first), false);
        }
    }
}
bool tallyTxBallot(const CVoteBallot &txBallot, const uint64_t &nCoins, const bool &undo)
{
    PollStack::iterator pIt = vIndex->pollCache.find(txBallot.PollID);
    if (pIt != vIndex->pollCache.end() && txBallot.OpSelection <= pIt->second.nTally.size() && txBallot.OpSelection > 0)
    {
        CVoteTally &tallyTx = *(pIt->second.nTally.begin()+txBallot.OpSelection -1);
        tallyTx.D4L = undo ? tallyTx.D4L -(nCoins/COIN) : tallyTx.D4L + (nCoins/COIN);

        LOCK(vIndex->cs_wallet);
        CVoteDB(vIndex->strWalletFile).WriteVote(pIt->second, false);
    } else {
        return false;
    }
    return true;
}
