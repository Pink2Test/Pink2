// Copyright (c) 2018 Project Orchid
// Copyright (c) 2018 The Pinkcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "votedb.h"
#include "vote.h"
#include "init.h"
#include "util.h"
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

    assert(poll.Option.size() == poll.nTally.size());

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
    return false;
}

bool CVotePoll::haveParent()
{
    if (Option.size() < 1)
        return false;

    vector<CPollOption>::iterator pIt = Option.begin();
    CPollID ParentID = stoi(*pIt);

    return (ParentID != 0 && vIndex->pollCache.find(ParentID) != vIndex->pollCache.end());

}
bool CVotePoll::parentEnded()
{
    if (Option.size() < 1)
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
    if (Name.size() < 2 || Question.size() < 6 || End <= Start ||
        ID == 0 || strlen((char *)vchPubKey.data()) < 33)
        return false;

    if (Option.size() > 1 && nHeight > 0 && hash > 0)
        return true;
    if (isFund() && nHeight > 0 && hash > 0)
        return true;

    if (isFund() && Option.size() > 0 && isLocal())
        return true;
    if (Option.size() > 1 && isLocal())
        return true;

    return false;
}

bool CVotePoll::isMine()
{
    // Is it valid? Do I have the private key for it? If not, is the poll local only (off chain)?
    return ((CPubKey(vchPubKey).IsValid() && pwalletMain->HaveKey(CPubKey(vchPubKey).GetID())) ||
            isLocal());
}

bool CVotePoll::isLocal()
{
    // If we're not in the Cache and Our ID is set then we MUST be local.
    return (ID != 0 && vIndex->pollCache.find(ID) == vIndex->pollCache.end());
}

bool CVotePoll::isConsensus()
{
    vector<CPollOption>::const_iterator pOpt = Option.begin();

    if(pOpt == Option.end() || Option.size() != 3 || nTally.size() != 3)
        return false;

    pOpt++; // Option 1 is reserved for PubKey if it's a claim poll;
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


// Because c++ doesn't like comparing structures/classes.
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
    if (pollCache.find(pollID) != pollCache.end())
    {
        current.setActive(pollCache.find(pollID), current.bIt, ActivePoll::SET_POLL);
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

    if (fromBlockchain)
        return p->isValid();

    readyPoll = true;

    if (p->Start < p->End)
        readyPoll = false;
    if (p->isClaim() && !fTestNet)
    {
        if(p->Start < GetPollTime2(GetAdjustedTime() + (24 * 7))) { readyPoll = false; }
        else if (p->Start < p->End + (24 * 14)) { readyPoll = false; }
        else if (!p->haveParent() || !p->parentEnded()) { readyPoll = false; }
    }

    delete p;

    return readyPoll;
}

bool CVote::commitPoll(const CVotePoll *poll, const bool &fromBlockchain)
{
    bool ret = CVoteDB(this->strWalletFile).WriteVote(*poll, !fromBlockchain);
    if (ret)
        this->pollStack.insert(make_pair(poll->ID, *poll));

    if (!fromBlockchain)
        return commitToChain(poll);

    return ret;
}

bool CVote::commitToChain(const CVotePoll* poll)
{
    return false; // we're getting there, but not yet.
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

    VDBErrors nLoadVoteDBRet = CVoteDB(strWalletFile,"cr+").LoadVote(this);

    if (nLoadVoteDBRet != VDB_LOAD_OK)
        return nLoadVoteDBRet;

    NewThread(ThreadFlushVoteDB, &strWalletFile);
    return VDB_LOAD_OK;
}

bool processRawPoll(const vector<CRawPoll>& rawPoll, const uint256& hash, const int& nHeight, const bool &checkOnly)
{
    
    // Poll Construction - OP_VOTE(1) + POLLID(4) + FLAGS(1) + START(2) + END(2) + SIZE_OF_COMPRESSED_DATA

    // Check that we have a good poll.
    vector<CRawPoll>::const_iterator rIt = rawPoll.begin();
    uint8_t opCount = ((rIt->n >> 4) & (uint8_t)7); // 0b00000111

    // Filthy hack to ensure consistency regardless of Big/Little Endian. Do Something better later.
    char b[2] { 0, 0 };
    memcpy(&b[0], &rIt->n, 2); b[0] &= 0x0F;

    uint16_t dataSize;
    dataSize = (((uint16_t)b[0]) << 8) + b[1];

    const size_t pollSize = POLL_HEADER_SIZE + dataSize; // 1(infoBit) + 4(PollID) + 20(Name) + 100(Question) + 2(Start) + 2(End) + 1(flags) + (OpCount * Size_Of_Options)
    if (pollSize > MAX_VOTE_SIZE) { printf("[processRawPoll: Poll Exceeds limit. \n"); return false; }
    if (rawPoll.size() != pollSize) { printf("[processRawPoll: Poll has invalid size.\n"); return false; }
    // if (((pollFlags >> 1) & (uint8_t)120) != critFlags) { printf("[processRawPoll]: Reported Critical Flags do not match raw poll."); return false; }
    bool success = true;
    CVotePoll *checkPoll = new CVotePoll();

    rIt += POLL_INFO_SIZE;

    if (!checkOnly)
    {
        try {

            success = false;

            memcpy(&checkPoll->ID, &rIt->n, POLL_ID_SIZE);
            rIt += POLL_ID_SIZE;

            memcpy(&checkPoll->Flags, &rIt->n, POLL_FLAG_SIZE); rIt += POLL_FLAG_SIZE;
            memcpy(&checkPoll->Start, &rIt->n, POLL_TIME_SIZE); rIt += POLL_TIME_SIZE;
            memcpy(&checkPoll->End, &rIt->n, POLL_TIME_SIZE ); rIt += POLL_TIME_SIZE;

            vector<char> dCharPoll;
            vector<char> cCharPoll(&rawPoll.begin()->c + POLL_HEADER_SIZE, &rawPoll.begin()->c + POLL_HEADER_SIZE + dataSize);

            charZip(cCharPoll, dCharPoll, true);
            vector<CRawPoll>dRawPoll;
            dRawPoll.resize(dCharPoll.size());
            memcpy(&dRawPoll.begin()->c, &*dCharPoll.data(), dCharPoll.size());
            vector<CRawPoll>::const_iterator cIt = dRawPoll.begin();

            if (dRawPoll.size() != POLL_NAME_SIZE + POLL_QUESTION_SIZE + (opCount * POLL_OPTION_SIZE) + POLL_PKEY_SIZE)
            { printf("[Process RawPoll]: Uncompressed does not match initially reported size.\n"); return false; }

            char charName[POLL_NAME_SIZE];
            memcpy(&charName, &cIt->c, POLL_NAME_SIZE);
            checkPoll->Name = charName;
            cIt += POLL_NAME_SIZE;

            char charQuestion[POLL_QUESTION_SIZE];
            memcpy(&charQuestion, &cIt->c, POLL_QUESTION_SIZE);
            checkPoll->Question = charQuestion;
            cIt += POLL_QUESTION_SIZE;

            for ( uint8_t i = 0 ; i < opCount ; i++ )
            {
                char pOpt[POLL_OPTION_SIZE];
                memcpy(&pOpt, &cIt->c, POLL_OPTION_SIZE);
                checkPoll->Option.push_back(string(pOpt));
                checkPoll->nTally.push_back(*(new CVoteTally));
                cIt += POLL_OPTION_SIZE;
            }

            unsigned char charPubKey[POLL_PKEY_SIZE];
            memcpy(&charPubKey, &cIt->n, POLL_PKEY_SIZE);

            vector<unsigned char> holdKey(&charPubKey[0], &charPubKey[0] + POLL_PKEY_SIZE);
            checkPoll->vchPubKey = holdKey;
            checkPoll->vchPubKey.resize(strlen((char *)holdKey.data()));


            checkPoll->OpCount = checkPoll->Option.size();
            checkPoll->hash = hash;
            checkPoll->nHeight = nHeight;

            if (vIndex->validatePoll(checkPoll))
                success = vIndex->commitPoll(checkPoll);

            printf("Success. %u %s %s %u %u. \n", checkPoll->ID, checkPoll->Name.c_str(), checkPoll->Question.c_str(), checkPoll->Start, checkPoll->End);

            return success;
        } catch (...) {
            printf("[processRawPoll]: Failed to process raw poll. Debug or contact your friendly neighborhood Dev for assistance./n");
            return false;
        }
    }

    return true;
}
bool getRawPoll(vector<CRawPoll>& rawPoll)
{
    CVotePoll ourPoll;

    ourPoll.pollCopy(*vIndex->current.poll);

    ourPoll.Name.resize(POLL_NAME_SIZE);
    ourPoll.Question.resize(POLL_QUESTION_SIZE);
    for (uint8_t i = 0; i < ourPoll.OpCount; i++)
        ourPoll.Option[i].resize(POLL_OPTION_SIZE);
    ourPoll.vchPubKey.resize(POLL_PKEY_SIZE);

    rawPoll.resize(MAX_VOTE_SIZE);
    vector<CRawPoll>::iterator rIt = rawPoll.begin();

    // rIt->n = (vIndex->current.poll->OpCount << 4);

    rIt += POLL_INFO_SIZE;

    memcpy(&rIt->n, &ourPoll.ID, POLL_ID_SIZE); rIt += POLL_ID_SIZE;
    memcpy(&rIt->n, &ourPoll.Flags, POLL_FLAG_SIZE); rIt += POLL_FLAG_SIZE;
    memcpy(&rIt->n, &ourPoll.Start, POLL_TIME_SIZE); rIt += POLL_TIME_SIZE;
    memcpy(&rIt->n, &ourPoll.End, POLL_TIME_SIZE); rIt += POLL_TIME_SIZE;

    vector<CRawPoll>crushRaw;
    crushRaw.resize(POLL_NAME_SIZE + POLL_QUESTION_SIZE + (ourPoll.OpCount * POLL_OPTION_SIZE) + POLL_PKEY_SIZE);
    vector<CRawPoll>::iterator cIt = crushRaw.begin();

    memcpy(&cIt->c, &*ourPoll.Name.c_str(), POLL_NAME_SIZE); cIt += POLL_NAME_SIZE;
    memcpy(&cIt->c, &*ourPoll.Question.c_str(), POLL_QUESTION_SIZE); cIt += POLL_QUESTION_SIZE;

    for (vector<CPollOption>::const_iterator it = ourPoll.Option.begin(); it < ourPoll.Option.end(); it++)
    { memcpy(&cIt->c, &*it->c_str(), POLL_OPTION_SIZE); cIt += POLL_OPTION_SIZE; }

    memcpy(&cIt->n, &*ourPoll.vchPubKey.begin(), POLL_PKEY_SIZE);

    vector<char> crush(&crushRaw[0].c, &crushRaw[0].c + crushRaw.size());
    vector<char> boom;
    vector<char> check;

    charZip(crush, check);
    charZip(check, boom, true);

    int zSuccess = strcmp(crush.data(), boom.data());

    if (zSuccess == 0)
    {
        rIt -= POLL_HEADER_SIZE;
        int16_t compressedSize = (int16_t)check.size();

        // Filthy hack to ensure consistency regardless of Big/Little Endian. Do Something better later.
        char b[2]; b[0] = compressedSize >> 8; b[1] = (compressedSize & 0x00FF);

        memcpy(&rIt->n, &b[0], 2);
        rIt->n |= (1U << 7 );  //We are a VotePoll.
        rIt->n |= (ourPoll.OpCount << 4);
        rIt += POLL_HEADER_SIZE;
        memcpy(&rIt->c, &*check.data(), check.size());

        // vector<CRawPoll> temp(&rawPoll.begin(), &rawPoll.begin() + compressedSize + POLL_HEADER_SIZE);
        rawPoll.resize(compressedSize + POLL_HEADER_SIZE);
        // rawPoll = temp;
        return true;
    } else {
        printf("[getRawPoll]: Failed to compress poll. \n");
        return false;
    }
}
bool processRawBallots(const vector<unsigned char>& rawBallots, const bool &checkOnly)
{
    return false;
}
