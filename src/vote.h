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

static PollStack::iterator pollIterator;
static BallotStack::iterator ballotIterator;
struct ActivePoll
{

    CPollID ID;
    CVotePoll& poll;
    CPollID BID;
    CVoteBallot& ballot;
    PollStack::iterator& pollIt;
    BallotStack::iterator& ballotIt;

    ActivePoll() :  poll(*(new CVotePoll)), ballot(*(new CVoteBallot)),
        pollIt(*(new PollStack::iterator)), ballotIt(*(new BallotStack::iterator))
    { ID = 0; BID =0; }

    void active(const PollStack::iterator& pit = pollIterator, const BallotStack::iterator& bit = ballotIterator );
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
        //clear();

        strWalletFile = strVoteFileIn;
        fFileBacked = true;
    }



    enum flags : CPollFlags {
        POLL_ENFORCE_POS        = 0,           // POS Only. No other votes are allowed (default).
        POLL_ALLOW_POS          = (1U << 0),   // Custom Poll Requirements - Allow POS Votes
        POLL_ALLOW_FPOS         = (1U << 1),   // Allow FPOS Votes
        POLL_ALLOW_SIDE_STAKE   = (1U << 2),   // Allow Side-Stake Votes
        POLL_ALLOW_POW          = (1U << 3),   // Allow POW Votes
        POLL_ALLOW_D4L          = (1U << 4),   // Allow Votes with D4L Donations - Minimum Donation 10 PINK.
        POll_ALLOW_TX10         = (1U << 5),   // Allow Transaction Votes (Transactions to Poll Originator). Minimum 10 PINK.
        POLL_BY_BLOCKHEIGHT     = (1U << 6),   // Use Blockheights instead of Blocktimes for start and end times. (100 Block Increments)
        POLL_VOTE_PER_ADDRESS   = (1U << 7),   // Only accept 1 vote per address.
    };

    void saveActive();
    bool newPoll(CVotePoll* poll, bool createPoll = false);
    bool setPoll(CPollID& pollID);
    bool getPoll(CPollID& pollID, CVotePoll& poll);

    CVotePoll getActivePoll();
    VDBErrors LoadVoteDB(bool& fFirstRunRet);

    PollStack pollCache;      // A cache of all the polls in the database.
    PollStack pollStack;      // These are our saved polls.
    BallotStack ballotStack;  // Our ballots for our saved polls.

    ActivePoll current;

};


int64_t GetPollTime(CPollTime& pTime, int& blockHeight = pindexBest->nHeight);
bool GetPollHeight(CPollID& pollID, int& pollHeight);
bool pollCompare(CVotePoll& a, CVotePoll& b);

#endif // VOTE_H
