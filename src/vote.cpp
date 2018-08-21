// Copyright (c) 2018 Project Orchid
// Copyright (c) 2018 The Pinkcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "votedb.h"
#include "vote.h"
#include "init.h"
#include <random>

CVote::CVote()
{
    clear();
}

void ActivePoll::setActive(PollStack::iterator &pit, BallotStack::iterator &bit, unsigned int flags)
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
         poll = new CVotePoll();
         ballot = new CVoteBallot();
     }

}

void CVote::clear()
{
    activePoll = new CVotePoll();
    activeBallot = new CVoteBallot();


    activePoll->ID = 0;
    activePoll->Name = "";
    activePoll->Question = "";
    activePoll->Flags = POLL_ENFORCE_POS;
    activePoll->Start = 0;
    activePoll->End = 0;
    activePoll->hash = 0;
    activePoll->nHeight = 0;
    activeBallot->PollID = 0;
    activeBallot->OpSelection = 0;

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

bool CVote::newPoll(CVotePoll* poll, bool createPoll)
{
    bool completePoll = true;
    // Save our current ballot & poll
    if (!createPoll)
        saveActive();

    if (poll->ID == 0)
        poll->ID = getNewPollID();

    CVote *blankVote = new CVote();
    CVotePoll *pointPoll = new CVotePoll();
    CVotePoll findPoll = *pointPoll;

/*
    if (poll == blankVote->activePoll)
    {   printf("[CVote::setPoll] Tried to set a blank poll, use clear() instead. \n"); return false; }

    if (poll->ID == blankVote->activePoll->ID)
    {   printf("[CVote::setPoll] Tried to set a poll without a poll ID. \n"); return false; }
*/
    if (getPoll(poll->ID, findPoll)) // Have Poll
    {
        CVotePoll thisPoll = *poll;
        if (!pollCompare(findPoll, thisPoll))  // Polls with the same PollID must be identical.
        {
            poll->ID = getNewPollID();
            newPoll(poll, true);
            return newPoll(poll);
        }

        // Shouldn't get this far, but if for some strange reason the poll we are setting does
        // match what we already have cached, then we'll load it like we're getPoll().
        activePoll = poll;
        activeBallot->PollID = poll->ID;
        activeBallot->OpSelection = 0;

        if (pollStack.find(poll->ID) == pollStack.end())        // Add to our saved polls
            pollStack.insert(make_pair(poll->ID, *poll));

        if (ballotStack.find(poll->ID) == ballotStack.end())    // Add to our saved ballots
            ballotStack.insert(make_pair(poll->ID, *activeBallot));
        else
            activeBallot->OpSelection = ballotStack.at(poll->ID).OpSelection;  // Load saved selection

        havePoll = true;
        return true;
    }

    CVotePoll *blankPoll = blankVote->activePoll;

    if ((poll->Start == blankPoll->Start || GetPollTime(poll->Start) < GetTime()) && createPoll != true)
    {   printf("[CVote::setPoll] Poll Start time is unset or in the past. \n"); completePoll = false; };

    if (poll->End > poll->Start + 2400 && createPoll != true)
    {   printf("[CVote::setPoll] Polls are limited to 100 days in length. \n"); completePoll = false; }

    if (poll->End < poll->Start && createPoll != true)
    {   printf("[CVote::setPoll] Attempted to set poll end after it begins. \n"); completePoll = false; }

    if (poll->Name == blankPoll->Name && createPoll != true)
    {   printf("[CVote::setPoll] Poll must have a name.\n"); completePoll = false; }

    if (poll->Question == blankPoll->Question && createPoll != true)
    {   printf("[CVote::setPoll] Poll must have a question to answer. \n"); completePoll = false; }

    poll->OpCount = 0;
    BOOST_FOREACH(CPollOption& pOption, poll->Option)
    {
        CPollOption blankOption;
        blankOption.clear();
        // blankOption.resize(POLL_OPTION_SIZE);
        if (pOption != blankOption)
            poll->OpCount++;
    }

    if ((poll->OpCount < 2 || poll->OpCount > 8) && createPoll != true)
    {
        printf("[CVote::setPoll] Option count out of range, must have 2-8 options to select. \n");
        completePoll = false;
    }

    activePoll = poll;    // successfully set a new poll.
    activeBallot->PollID = poll->ID;
    activeBallot->OpSelection = 0;

    if (pollStack.find(poll->ID) == pollStack.end())        // Add to our saved polls
        pollStack.insert(make_pair(poll->ID, *poll));

    if (ballotStack.find(poll->ID) == ballotStack.end())    // Add to our saved ballots
        ballotStack.insert(make_pair(poll->ID, *activeBallot));
    else
        activeBallot->OpSelection = ballotStack.at(poll->ID).OpSelection;  // Load saved selection

    havePoll = true;
    return completePoll;
}

void CVote::saveActive()
{
    if (ballotStack.find(activeBallot->PollID) != ballotStack.end() && activeBallot->PollID != 0)
        ballotStack.at(activeBallot->PollID) = *activeBallot;
    else if (activeBallot->PollID != 0)
        ballotStack.insert(make_pair(activeBallot->PollID, *activeBallot));

    if (pollStack.find(activePoll->ID) != pollStack.end() && activePoll->ID != 0)
        pollStack.at(activePoll->ID) = *activePoll;
    else if (activePoll->ID != 0)
        pollStack.insert(make_pair(activePoll->ID, *activePoll));
}

bool CVote::getPoll(CPollID& pollID, CVotePoll& poll)
{
    // Save our current ballot and poll
    saveActive();

    if (pollStack.find(pollID) != pollStack.end())
    {
        poll = pollStack.at(pollID);
        return true;
    } else if (pollCache.find(pollID) != pollCache.end())
    {
        poll = pollCache.at(pollID);
        return true;
    }
    return false;
}


// Because c++ doesn't like comparing structures/classes.
bool pollCompare(CVotePoll& a, CVotePoll& b)
{
    if (a.ID != b.ID || a.Name != b.Name ||
        a.Question != b.Question || a.Start != b.Start ||
        a.End != b.End || a.Flags != b.Flags || a.hash != b.hash ||
        a.nHeight != b.nHeight)
    return false;

    for (unsigned int i=0 ; i < a.Option.size() ; i++)
    {
        if (a.Option[i] != b.Option[i])
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
    if (pollCache.find(pollID) != pollCache.end())
    {
        *activePoll = pollCache.at(pollID);
        havePoll = true;
        return true;
    }
    if (pollStack.find(pollID) != pollStack.end())
    {
        *activePoll = pollStack.at(pollID);
        havePoll = true;
        return true;
    }

    return false; // didn't find a poll to set.
}

CVotePoll CVote::getActivePoll()
{
    return *activePoll;
}

int64_t GetPollTime(CPollTime &pTime, int &blockHeight)
{

    /* If we're within 10k blocks of our 4 BLOCKCOUNT years cutoff,
     * interpret the time based on pTime value restrictions.
     * This is important since we can't be sure when the poll will
     * first be accepted into a block when it is created and submitted
     * to the mempool. 10k blocks gives about a week of padding.     */
    if (blockHeight - 10000 < (YEARLY_BLOCKCOUNT * 4) && blockHeight + 10000 > (YEARLY_BLOCKCOUNT * 4))
    {
        if(pTime <= 2400) // 100 days past baseTime.
            blockHeight = blockHeight + 10000;   // pTime is set too far behind to be pre-YB*4 at this blockheight.
        else
            blockHeight = blockHeight - 10000;   // pTime is set too far ahead to be post-YB*4 at this blockheight.
    }


    /* Reset PollTime every 4 BLOCKCOUNT years.
     * CPollTime is incremented every hour since the first hour
     * after GenesisBlock So it can contain 7 1/2 years of time. */
    int baseClock = blockHeight / (YEARLY_BLOCKCOUNT * 4);
    int baseHeight = baseClock * (YEARLY_BLOCKCOUNT * 4);

    CBlockIndex *tIndex = FindBlockByHeight(baseHeight);

    int64_t baseTime = tIndex->nTime / 60 / 60;
    int64_t pollTime = (baseTime + (int64_t)pTime) * 60 * 60;

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
