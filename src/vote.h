// Copyright (c) 2018 Project Orchid
// Copyright (c) 2018 The Pinkcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef VOTE_H
#define VOTE_H

#include <wallet.h>
#include <main.h>
#include <votedb.h>

using namespace std;

static const uint64_t POLL_CONSENSUS_PRECISION = 10000000;

static const size_t POLL_ID_SIZE        = 4;
static const size_t POLL_NAME_SIZE      = 20;
static const size_t POLL_QUESTION_SIZE  = 100;
static const size_t POLL_OPTION_SIZE    = 45;
static const size_t POLL_FLAG_SIZE      = 1;
static const size_t POLL_TIME_SIZE      = 2;
static const size_t POLL_OPTION_COUNT   = 6;
static const size_t POLL_ADDRESS_SIZE   = 33; // All polls use PubKeyHash, aka Address.
static const size_t POLL_INFO_SIZE      = 2U;
static const size_t POLL_HEADER_SIZE    = POLL_INFO_SIZE + POLL_ID_SIZE + POLL_FLAG_SIZE + (2*POLL_TIME_SIZE);


typedef uint32_t CPollID;
typedef string CPollName;
typedef uint16_t CPollTime;
typedef string CPollQuestion;
typedef uint8_t COptionID;
typedef string CPollOption;
typedef uint8_t CPollFlags;

struct CVoteBallot
{
    CVoteBallot() { clear(); }
    ~CVoteBallot() {}

    void clear() {PollID = 0; OpSelection = 0;}
    CPollID PollID;
    COptionID OpSelection;
};

struct CVoteTally
{
    uint32_t POS;
    uint32_t FPOS;
    uint32_t POW;
    uint32_t D4L;

    CVoteTally(){ POS =0; FPOS=0; POW=0; D4L=0; }
    ~CVoteTally() {}
};

struct CVotePoll
{
    CVotePoll(){ clear(); }
    ~CVotePoll() {}

    void pollCopy(const CVotePoll& poll);

    void clear() {ID = 0; Name = ""; Flags = 0; Start = 0; End = 0; Question = ""; OpCount = 0; hash = 0; nHeight = 0;}
    CPollID ID;                             // 4 Bytes
    CPollName Name;                         // 20 Bytes
    CPollFlags Flags;                       // 1 Byte
    CPollTime Start;                        // 2 Bytes
    CPollTime End;                          // 2 Bytes
    CPollQuestion Question;                 // 100 Bytes
    COptionID OpCount;                      // 1 Byte
    vector<CPollOption> Option;             // 45 bytes * 6(max) = 270 bytes

    uint256 hash;                           // Store the poll's transaction information.
    uint64_t nHeight;                       // Set the height that the poll got accepted into the blockchain.
    vector<CVoteTally> nTally;              // Store the current tally on the blockchain.
    vector<unsigned char> vchPubKey;        // Pubkey of Poll Creator.

    bool onlyPOS();
    bool acceptPOS();
    bool acceptFPOS();
    bool acceptPOW();
    bool acceptD4L();
    bool isFund();
    bool isBounty();
    bool isP2POLL();
    bool isClaim();
    bool isConsensus();
    bool isComplete();
    bool isLocal();
    bool isMine();
    bool isValid();
    bool hasEnded();
    bool haveParent();
    bool parentEnded();
    bool isApproved();
    bool isFullyApproved();

private:
    uint64_t getConsensus();
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

    ActivePoll(); 
    ~ActivePoll() {}

    void setActive(const PollStack::iterator &pit, const BallotStack::iterator &bit, unsigned int flags = SET_POLL_AND_BALLOT);

private:
};

class CVote : public CWallet
{
private:
    bool readyPoll;          // Poll is in the chain AND in our local stack.
    uint8_t optionCount;
    uint8_t ballotCount;

    CVotePoll getPollByID(CPollID* pollID);
    CVotePoll getPollByName(CPollName* pollName);
    CVotePoll getPollByDesc(string* pollDesc);
    CPollID getNewPollID();

public:
    void clear();
    CVote();
    ~CVote() {}

    CVote(std::string strVoteFileIn)
    {
        clear();
        strWalletFile = strVoteFileIn;
        fFileBacked = true;
    }

    enum flags : CPollFlags {
        POLL_ENFORCE_POS        = 0,           // POS Only. No other votes bool altare allowed (default).
        POLL_ALLOW_POS          = (1U << 0),   // Custom Poll Requirements - Allow POS Votes
        POLL_ALLOW_FPOS         = (1U << 1),   // Allow FPOS Votes
        POLL_ALLOW_POW          = (1U << 2),   // Allow POW Votes
        POLL_ALLOW_D4L          = (1U << 3),   // Allow Votes with D4L Donations - Minimum Donation 10 PINK.
        POLL_PAY_TO_POLL        = (1U << 4),   // Funds raised through funds are held until they are voted to be released.
        POLL_FUNDRAISER         = (1U << 5),   // Fundraiser poll. Reset poll fields, treat as Fundraiser.
        POLL_BOUNTY             = (1U << 6) |
                                  POLL_PAY_TO_POLL,  // Bounty poll. Reset poll fields, treat as Bounty.
        POLL_CLAIM              = (1U << 7)       |  // Bounty/Fundraiser claim poll. Force sets POS, FPOS, and POW.
                                  POLL_ALLOW_POS  |
                                  POLL_ALLOW_FPOS |
                                  POLL_ALLOW_POW
    };

    bool newPoll(CVotePoll* poll);
    bool setPoll(CPollID& pollID);
    bool getPoll(CPollID& pollID, CVotePoll* poll);
    bool validatePoll(const CVotePoll *poll, const bool& fromBlockchain = false);
    bool commitPoll(const CVotePoll *poll, const bool& fromBlockchain = false);
    bool commitToChain(const CVotePoll* poll);

    CVotePoll getActivePoll();
    VDBErrors LoadVoteDB(bool& fFirstRunRet);

    PollStack pollCache;      // A cache of all the polls in the database.
    PollStack pollStack;      // These are our saved polls.
    BallotStack ballotStack;  // Our ballots for our saved polls.

    ActivePoll current;
};


int64_t GetPollTime(const CPollTime &pTime, const int &blockHeight = pindexBest->nHeight);
CPollTime GetPollTime2(const int64_t& uTime, const int& blockHeight = pindexBest->nHeight);
bool GetPollHeight(CPollID& pollID, int& pollHeight);
bool pollCompare(CVotePoll* a, CVotePoll* b);
bool processRawPoll(const vector<unsigned char> &rawPoll, const uint256 &hash, const int &nHeight, const bool &checkOnly = true);
bool processRawBallots(const vector<unsigned char>& rawBallots, const bool &checkOnly = true);
bool getRawPoll(vector<unsigned char>& rawPoll);


#endif // VOTE_H
