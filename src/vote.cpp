// Copyright (c) 2018 Project Orchid
// Copyright (c) 2018 The Pinkcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "votedb.h"
#include "vote.h"
#include "init.h"
#include "zlib.h"
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

    for (vector<CPollOption>::const_iterator i = poll.Option.begin(); i < poll.Option.end() ; i++)
    {
        Option.push_back(*i);
    }

    nHeight = poll.nHeight;
    nTally = poll.nTally;
    hash = poll.hash;
}

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

    this->havePoll = false;
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

    havePoll = false;
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

    for (unsigned int i=0 ; i < a->Option.size() ; i++)
    {
        if (a->Option[i] != b->Option[i])
            return false;
    }
    return true;
}

CPollID CVote::getNewPollID()
{
    random_device rd;
    default_random_engine random(rd());
    uniform_int_distribution<uint32_t> range(0, 0xFFFFFFFF);

    CPollID randomID = range(random);

    return randomID;
}

bool CVote::setPoll(CPollID& pollID)
{
    bool setPoll = false;
    if (pollCache.find(pollID) != pollCache.end())
    {
        current.setActive(pollCache.find(pollID), current.bIt, ActivePoll::SET_POLL);
        havePoll = true;
        setPoll=true;
    }
    if (pollStack.find(pollID) != pollStack.end())
    {
        current.setActive(pollStack.find(pollID), current.bIt, ActivePoll::SET_POLL);
        havePoll = true;
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

bool CVote::validatePoll(const CVotePoll *poll, const bool& fromBlockchain)
{
    return false;
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

    // Check that we have a good poll.
    vector<CRawPoll>::const_iterator cIt = rawPoll.begin();
    uint8_t opCount = (cIt->n & (uint8_t)7); // 0b00000111
    uint8_t critFlags = (cIt->n & (uint8_t)120); //0b01111000

    cIt += (1U + 4U + 2U + 2U);
    uint8_t pollFlags = cIt->n;
    const size_t pollSize = 130 + (45 * opCount); // 1(infoBit) + 4(PollID) + 20(Name) + 100(Question) + 2(Start) + 2(End) + 1(flags) + (OpCount * Size_Of_Options)
    if (pollSize > MAX_VOTE_SIZE) { printf("[processRawPoll: Poll Exceeds limit. \n"); return false; }
    if (rawPoll.size() != pollSize) { printf("[processRawPoll: Poll has invalid size.\n"); return false; }
    if (((pollFlags >> 1) & (uint8_t)120) != critFlags) { printf("[processRawPoll]: Reported Critical Flags do not match raw poll."); return false; }

    cIt -= (4U + 2U + 2U);
    bool success = true;
    CVotePoll *checkPoll = new CVotePoll();

    if (!checkOnly)
    {
        try {

            success = false;

            memcpy(&checkPoll->ID, &cIt->n, POLL_ID_SIZE);
            cIt += POLL_ID_SIZE;

            memcpy(&checkPoll->Start, &cIt->n, 2U); cIt += 2U;
            memcpy(&checkPoll->End, &cIt->n, 2U ); cIt += 2U;
            memcpy(&checkPoll->Flags, &cIt->n, 1U); cIt += 1U;

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
                cIt += POLL_OPTION_SIZE;
            }


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
bool processRawBallots(const vector<unsigned char>& rawBallots, const bool &checkOnly)
{
    return false;
}
