// Copyright (c) 2018 Project Orchid
// Copyright (c) 2018 The Pinkcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef VOTE_H
#define VOTE_H

#include <wallet.h>
#include <main.h>
#include <base58.h>
#include <votedb.h>


using namespace std;

const size_t POLL_ID_SIZE        = 4;
const size_t POLL_NAME_SIZE      = 15;
const size_t POLL_QUESTION_SIZE  = 100;
const size_t POLL_OPTION_SIZE    = 45;

typedef uint32_t CPollID;
typedef string CPollName;
typedef uint16_t CPollTime;
typedef string CPollQuestion;
typedef uint8_t COptionID;
typedef string CPollOption;
typedef uint8_t CPollFlags;
// typedef map<CPollID, COptionID> CVoteBallot;

typedef union { unsigned char key[4]; uint32_t n; } CPollIDByte;
typedef union { unsigned char key[2]; uint16_t n; } CPollTimeByte;
typedef union { unsigned char key[1]; uint8_t n; } CPollOpCountByte;
typedef union { unsigned char key[1]; uint8_t n; } CPollFlagsByte;

struct CVoteBallot
{
    CPollID PollID;
    COptionID OpSelection;
};

struct CVotePoll
{
    CVotePoll(){ clear(); }

    void clear() {ID = 0; Name = ""; Flags = 0; Start = 0; End = 0; Question = ""; OpCount = 0; hash = 0; nHeight = 0;}
    CPollID ID;                             // 4 Bytes
    CPollName Name;                         // 15 Bytes
    CPollFlags Flags;                       // 1 Byte
    CPollTime Start;                        // 2 Bytes
    CPollTime End;                          // 2 Bytes
    CPollQuestion Question;                 // 100 Bytes
    COptionID OpCount;                      // 1 Byte
    vector<CPollOption> Option;             // 45 bytes * 8(max) = 360 bytes

    uint256 hash;                      // Store the poll's transaction information.
    uint64_t nHeight;                       // set the height that the poll got accepted into the blockchain.
};

typedef union { unsigned char b[8]; uint64_t n; } nHeightByte;

typedef map<CPollID, CVotePoll> PollStack;
typedef map<CPollID, CVoteBallot> BallotStack;

enum APFlags {
    SET_CLEAR = 0,
    SET_POLL = (1U << 0),
    SET_BALLOT = (1U << 1),
    SET_POLL_AND_BALLOT = SET_POLL | SET_BALLOT
};

struct ActivePoll
{

    enum {
        SET_CLEAR = 0,
        SET_POLL = (1U << 0),
        SET_BALLOT = (1U << 1),
        SET_POLL_AND_BALLOT = SET_POLL | SET_BALLOT
    };

    CPollID ID;
    CVotePoll* poll;
    CPollID BID;
    CVoteBallot* ballot;
    PollStack::iterator& pIt;
    BallotStack::iterator& bIt;

    ActivePoll() :  pIt(*(new PollStack::iterator)), bIt(*(new BallotStack::iterator))
    { ID = 0; BID =0; setActive(pIt, bIt, SET_CLEAR); poll->clear();}

    void setActive(const PollStack::iterator &pit, const BallotStack::iterator &bit, unsigned int flags = SET_POLL_AND_BALLOT);
};

class CVote : public CWallet
{
private:
    CVotePoll *activePoll;
    CVoteBallot *activeBallot;

    bool havePoll;
    uint8_t optionCount;
    uint8_t ballotCount;

    CVotePoll getPollByID(CPollID* pollID);
    CVotePoll getPollByName(CPollName* pollName);
    CVotePoll getPollByDesc(string* pollDesc);
    CPollID getNewPollID();

public:
    mutable CCriticalSection cs_vote;

    void clear();
    CVote();

    CVote(std::string strVoteFileIn)
    {
        clear();
        strWalletFile = strVoteFileIn;
        fFileBacked = true;
    }

    enum flags : CPollFlags {
        POLL_ENFORCE_POS        = 0,           // POS Only. No other votes are allowed (default).
        POLL_ALLOW_POS          = (1U << 0),   // Custom Poll Requirements - Allow POS Votes
        POLL_ALLOW_FPOS         = (1U << 1),   // Allow FPOS Votes
        POLL_ALLOW_POW          = (1U << 2),   // Allow POW Votes
        POLL_ALLOW_D4L          = (1U << 3),   // Allow Votes with D4L Donations - Minimum Donation 10 PINK.
        POLL_VOTE_PER_ADDRESS   = (1U << 4),   // Only accept 1 vote per address.
        POLL_FUNDRAISER         = (1U << 5),   // Fundraiser poll. Reset poll fields, treat as Fundraiser.
        POLL_BOUNTY             = (1U << 6),   // Bounty poll. Reset poll fields, treat as Bounty.
        POLL_CLAIM              = (1U << 7)       |  // Bounty/Fundraiser claim poll. Force sets POS, FPOS, and POW.
                                  POLL_ALLOW_POS  |
                                  POLL_ALLOW_FPOS |
                                  POLL_ALLOW_POW
    };

    void saveActive();
    bool newPoll(CVotePoll* poll, bool createPoll = false);
    bool setPoll(CPollID& pollID);
    bool getPoll(CPollID& pollID, CVotePoll* poll);

    CVotePoll getActivePoll();
    VDBErrors LoadVoteDB(bool& fFirstRunRet);

    PollStack pollCache;      // A cache of all the polls in the database.
    PollStack pollStack;      // These are our saved polls.
    BallotStack ballotStack;  // Our ballots for our saved polls.

    ActivePoll current;
};


int64_t GetPollTime(const CPollTime &pTime, const int &blockHeight = pindexBest->nHeight);
CPollTime GetPollTime(const int64_t& uTime, const int& blockHeight = pindexBest->nHeight);
bool GetPollHeight(CPollID& pollID, int& pollHeight);
bool pollCompare(CVotePoll* a, CVotePoll* b);



#endif // VOTE_H
