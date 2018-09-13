// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "votedb.h"
#include "vote.h"
#include "init.h"

#include <boost/version.hpp>
#include <boost/filesystem.hpp>

using namespace std;
using namespace boost;

//
// CVoteDB
//

bool CVoteDB::WriteVote(const CVotePoll& votePoll, bool isLocal)
{
    bool success = true;

    string Local = "";
    if (isLocal)
        Local = "Local";

    string name = "name" + Local;
    string flags = "flags" + Local;
    string start = "start" + Local;
    string end = "end" + Local;
    string question = "question" + Local;
    string txhash = "txhash";
    string height = "height";
    string strAddress = "address" + Local;

    string option[7];
    string tallyPOS[7];
    string tallyFPOS[7];
    string tallyPOW[7];
    string tallyD4L[7];

    for (int i = 0; i < 7 ; i++ )
    {
        option[i] = "option" + to_string(i) + Local;
        if (Local.size() == 0)
        {
            tallyPOS[i] = "tPOS" + to_string(i);
            tallyFPOS[i] = "tFPOS" + to_string(i);
            tallyPOW[i] = "tPOW" + to_string(i);
            tallyD4L[i] = "tD4L" + to_string(i);
        }
    }


    string sPollID = std::to_string(votePoll.ID);
    string sPollStart = std::to_string(votePoll.Start);
    string sPollEnd = std::to_string(votePoll.End);
    string sPollFlags = std::to_string(votePoll.Flags);
    string sPollHeight = std::to_string(votePoll.nHeight);
    string sHash = votePoll.hash.GetHex();


    if (!Write(make_pair(name, sPollID), votePoll.Name))
        success = false;
    if (!Write(make_pair(flags, sPollID), sPollFlags))
        success = false;
    if (!Write(make_pair(start, sPollID), sPollStart))
        success = false;
    if (!Write(make_pair(end, sPollID), sPollEnd))
        success = false;
    if (!Write(make_pair(question, sPollID), votePoll.Question))
        success = false;
    if (!Write(make_pair(strAddress, sPollID), votePoll.strAddress))
        success = false;

    if (!isLocal) {
        if (!Write(make_pair(txhash, sPollID), sHash))
            success = false;
        if (!Write(make_pair(height, sPollID), sPollHeight))
            success = false;
    }

    for (uint8_t i = votePoll.OpCount ; i > 0 ; i--)          // Peel them off in reverse so we can preserve order
    {                                                         // when we load them again later.
        const string vOption = votePoll.Option[i-1];
        if (!Write(make_pair(option[i], sPollID), vOption))
            success = false;

        if (!isLocal)
        {
            const string vTallyPOS = to_string(votePoll.nTally[i-1].POS);
            const string vTallyFPOS = to_string(votePoll.nTally[i-1].FPOS);
            const string vTallyPOW = to_string(votePoll.nTally[i-1].POW);
            const string vTallyD4L = to_string(votePoll.nTally[i-1].D4L);
            if (!Write(make_pair(tallyPOS[i], sPollID), vTallyPOS))
                success = false;
            if (!Write(make_pair(tallyFPOS[i], sPollID), vTallyFPOS))
                success = false;
            if (!Write(make_pair(tallyPOW[i], sPollID), vTallyPOW))
                success = false;
            if (!Write(make_pair(tallyD4L[i], sPollID), vTallyD4L))
                success = false;
        }
    }
    nVoteDBUpdated++;
    return success;
}

bool CVoteDB::EraseVote(const CVotePoll& votePoll, bool isLocal)
{

    bool success = false;

    string Local = "";
    if (isLocal)
        Local = "Local";

    string name = "name" + Local;
    string flags = "flags" + Local;
    string start = "start" + Local;
    string end = "end" + Local;
    string question = "question" + Local;
    string txhash = "txhash";
    string height = "height";
    string strAddress = "address" + Local;

    string option[7];
    string tallyPOS[7];
    string tallyFPOS[7];
    string tallyPOW[7];
    string tallyD4L[7];

    for (int i = 0; i < 7 ; i++ )
    {
        option[i] = "option" + to_string(i) + Local;
        if (!isLocal)
        {
            tallyPOS[i] = "tPOS" + to_string(i);
            tallyFPOS[i] = "tFPOS" + to_string(i);
            tallyPOW[i] = "tPOW" + to_string(i);
            tallyD4L[i] = "tD4L" + to_string(i);
        }
    }

    string sPollID = std::to_string(votePoll.ID);

    if (Erase(make_pair(name, sPollID)))
        success = true;
    if (Erase(make_pair(flags, sPollID)))
        success = true;
    if (Erase(make_pair(start, sPollID)))
        success = true;
    if (Erase(make_pair(end, sPollID)))
        success = true;
    if (Erase(make_pair(question, sPollID)))
        success = true;
    if (Erase(make_pair(strAddress, sPollID)))
        success = true;

    if (!isLocal)
    {
        if (Erase(make_pair(txhash, sPollID)))
            success = true;
        if (Erase(make_pair(height, sPollID)))
            success = true;
    }

    for (uint8_t i = votePoll.OpCount ; i > 0 ; i--)          // Peel them off in reverse so we can preserve order
    {                                                         // when we load them again later.
        if (Erase(make_pair(option[i], sPollID)))
            success = true;
        if (Local.size() == 0)
        {
                if (Erase(make_pair(tallyPOS[i], sPollID)))
                success = true;
                if (Erase(make_pair(tallyFPOS[i], sPollID)))
                success = true;
                if (Erase(make_pair(tallyPOW[i], sPollID)))
                success = true;
                if (Erase(make_pair(tallyD4L[i], sPollID)))
                success = true;
        }
    }

    nVoteDBUpdated++;

    return success;
}

bool CVoteDB::ReadVote(const CVotePoll& votePollID, CVotePoll& votePoll, bool isLocal)
{

    bool success = true;
    bool readAnything = false;

    string Local = "";
    if (isLocal)
        Local = "Local";

    string name = "name" + Local;
    string flags = "flags" + Local;
    string start = "start" + Local;
    string end = "end" + Local;
    string question = "question" + Local;
    string txhash = "txhash";
    string height = "height";
    string strAddress = "address" + Local;

    string option[7];
    string tallyPOS[7];
    string tallyFPOS[7];
    string tallyPOW[7];
    string tallyD4L[7];

    for (int i = 0; i < 7 ; i++ )
    {
        option[i] = "option" + to_string(i) + Local;
        if (Local.size() == 0)
        {
                tallyPOS[i] = "tPOS" + to_string(i);
                tallyFPOS[i] = "tFPOS" + to_string(i);
                tallyPOW[i] = "tPOW" + to_string(i);
                tallyD4L[i] = "tD4L" + to_string(i);
        }
    }

    string sPollID = std::to_string(votePollID.ID);
    string sPollStart;
    string sPollEnd;
    string sPollFlags;
    string sPollHeight;
    string sHash;

    votePoll.ID = votePollID.ID;

    if (!Read(make_pair(name, sPollID), votePoll.Name))
        success = false;
    else
        readAnything = true;
    if (!Read(make_pair(flags, sPollID), sPollFlags))
        success = false;
    else
    {
        if (sPollFlags != "")
            votePoll.Flags = (uint8_t)stoi(sPollFlags);
        readAnything = true;
    }
    if (!Read(make_pair(start, sPollID), sPollStart))
        success = false;
    else
    {
        if (sPollStart != "")
            votePoll.Start = (int16_t)stoi(sPollStart);
        readAnything = true;
    }
    if (!Read(make_pair(end, sPollID), sPollEnd))
        success = false;
    else
    {
        if (sPollEnd != "")
            votePoll.End = (int16_t)stoi(sPollEnd);
        readAnything = true;
    }
    if (!Read(make_pair(question, sPollID), votePoll.Question))
        success = false;
    else
        readAnything = true;
    if (!Read(make_pair(strAddress, sPollID), votePoll.strAddress))
        success = false;
    else
        readAnything = true;
    if (!Read(make_pair(txhash, sPollID), sHash))
        success = false;
    else
    {
        if (sHash != "")
            votePoll.hash.SetHex(sHash);
        readAnything = true;
    }
    if (!Read(make_pair(height, sPollID), sPollHeight))
        success = false;
    else
    {
        if (sPollHeight != "")
            votePoll.nHeight = stoul(sPollHeight);
        readAnything = true;
    }


    for (uint8_t i = 0 ; i < 6 ; i++)
    {
        string vOption;
        string sTally;
        CVoteTally vTally;
        bool tallySuccess = true;
        if (Read(make_pair(option[i+1], sPollID), vOption))
        {
            votePoll.Option.push_back(vOption);
            votePoll.OpCount = (int8_t)votePoll.Option.size();
            readAnything = true;
        }

        if (Read(make_pair(tallyPOS[i+1], sPollID), sTally))
            vTally.POS = stoul(sTally);
        else
            tallySuccess = false;

        if (Read(make_pair(tallyFPOS[i+1], sPollID), sTally))
            vTally.FPOS = stoul(sTally);
        else
            tallySuccess = false;

        if (Read(make_pair(tallyPOW[i+1], sPollID), sTally))
            vTally.POW = stoul(sTally);
        else
            tallySuccess = false;

        if (Read(make_pair(tallyD4L[i+1], sPollID), sTally))
            vTally.D4L = stoul(sTally);
        else
            tallySuccess = false;

        if (tallySuccess)
            votePoll.nTally.push_back(vTally);
    }

    if ((votePoll.OpCount != votePoll.Option.size()) || (votePoll.Option.size() != votePoll.nTally.size()))
        success = false;

    if ((votePoll.OpCount < 1 && votePoll.isFund()) && votePoll.OpCount < 2)
        success = false;

    if (readAnything && isLocal)
        success = true;

    return success;
}

bool CVoteDB::WriteBallot(const CVoteBallot& voteBallot)
{

    bool success = true;

    string sPollID = std::to_string(voteBallot.PollID);
    string sOpSelection = std::to_string(voteBallot.OpSelection);
    if (!Write(make_pair(string("ballotID"), sPollID), sOpSelection))
        success = false;

    nVoteDBUpdated++;
    return success;
}

bool CVoteDB::EraseBallot(const CVoteBallot& voteBallot)
{
    bool success=true;

    string sPollID = std::to_string(voteBallot.PollID);
    if (!Erase(make_pair(string("ballotID"), sPollID)))
        success = false;

    nVoteDBUpdated++;
    return success;
}

bool CVoteDB::ReadBallot(const CVoteBallot& vBallot, CVoteBallot& voteBallot)
{
    bool success = true;

    string sPollID = std::to_string(vBallot.PollID);
    string sOpSelection = "";
    if (!Read(make_pair(string("ballotID"), sPollID), sOpSelection))
        success = false;

    if (sOpSelection !="")
    {
        voteBallot.PollID = vBallot.PollID;
        voteBallot.OpSelection = (uint8_t)stoi(sOpSelection);
    }

    return success;
}

bool
ReadElementValue(CVote* voteIndex, CDataStream& ssKey, CDataStream& ssValue, string& strType, string& strErr)
{
    try {
        // Unserialize
        // Taking advantage of the fact that pair serialization
        // is just the two items serialized one after the other

        CVotePoll activePoll;
        CVoteBallot activeBallot;
        string strVoteID = "";

        ActivePoll stack;

        ssKey >> strType;

        if (ssKey.size() > 0)
        {
            ssKey >> strVoteID;

            activePoll.ID = (CPollID)stoul(strVoteID);
            activeBallot.PollID = activePoll.ID;

            int flags = ActivePoll::SET_CLEAR;
            stack.bIt = voteIndex->ballotStack.find(activeBallot.PollID);
            flags |= (stack.bIt != voteIndex->ballotStack.end()) ? ActivePoll::SET_BALLOT : ActivePoll::SET_CLEAR;

            if (strType.find("Local", 0) == string::npos)
            {
                stack.pIt = voteIndex->pollCache.find(activePoll.ID);
                flags |= (stack.pIt != voteIndex->pollCache.end()) ? ActivePoll::SET_POLL : ActivePoll::SET_CLEAR;
            }
            else
            {
                stack.pIt = voteIndex->pollStack.find(activePoll.ID);
                flags |= (stack.pIt != voteIndex->pollStack.end()) ? ActivePoll::SET_POLL : ActivePoll::SET_CLEAR;
            }

            stack.setActive(stack.pIt, stack.bIt, flags);
        }

        if (strType.rfind("name", 0) == 0)
        {
            ssValue >> activePoll.Name;

            size_t typeLen = strlen("name");
            if (strType.size() == typeLen)
            {
                if (stack.pIt != voteIndex->pollCache.end())
                    stack.poll->Name = activePoll.Name;
                else
                    voteIndex->pollCache.insert(make_pair(activePoll.ID, activePoll));
            } else if (strType.find("Local", typeLen) != string::npos)
            {
                if (stack.pIt != voteIndex->pollStack.end())
                    stack.poll->Name = activePoll.Name;

                else
                    voteIndex->pollStack.insert(make_pair(activePoll.ID, activePoll));
            }
        }
        else if (strType.rfind("flags", 0) == 0)
        {
            string strFlags;
            ssValue >> strFlags;
            activePoll.Flags = (CPollFlags)stoi(strFlags);

            size_t typeLen = strlen("flags");
            if (strType.size() == typeLen)
            {
                if (stack.pIt != voteIndex->pollCache.end())
                    stack.poll->Flags = activePoll.Flags;
                else
                    voteIndex->pollCache.insert(make_pair(activePoll.ID, activePoll));
            } else if (strType.find("Local", typeLen) != string::npos)
            {
                if (stack.pIt != voteIndex->pollStack.end())
                    stack.poll->Flags = activePoll.Flags;
                else
                    voteIndex->pollStack.insert(make_pair(activePoll.ID, activePoll));
            }
        }
        else if (strType.rfind("start", 0) == 0)
        {
            string strPollTime;
            ssValue >> strPollTime;
            activePoll.Start = (CPollTime)stoi(strPollTime);

            size_t typeLen = strlen("start");
            if (strType.size() == typeLen)
            {
                if (stack.pIt != voteIndex->pollCache.end())
                    stack.poll->Start = activePoll.Start;
                else
                    voteIndex->pollCache.insert(make_pair(activePoll.ID, activePoll));
            } else if (strType.find("Local", typeLen) != string::npos)
            {
                if (stack.pIt != voteIndex->pollStack.end())
                    stack.poll->Start = activePoll.Start;
                else
                    voteIndex->pollStack.insert(make_pair(activePoll.ID, activePoll));
            }
        }
        else if (strType.rfind("end", 0) == 0)
        {
            string strPollTime;
            ssValue >> strPollTime;
            activePoll.End = (CPollTime)stoi(strPollTime);

            size_t typeLen = strlen("end");
            if (strType.size() == typeLen)
            {
                if (stack.pIt != voteIndex->pollCache.end())
                    stack.poll->End = activePoll.End;
                else
                    voteIndex->pollCache.insert(make_pair(activePoll.ID, activePoll));
            } else if (strType.find("Local", typeLen) != string::npos)
            {
                if (stack.pIt != voteIndex->pollStack.end())
                    stack.poll->End = activePoll.End;
                else
                    voteIndex->pollStack.insert(make_pair(activePoll.ID, activePoll));
            }
        }
        else if (strType.rfind("question", 0) == 0)
        {
            ssValue >> activePoll.Question;

            size_t typeLen = strlen("question");
            if (strType.size() == typeLen)
            {
                if (stack.pIt != voteIndex->pollCache.end())
                    stack.poll->Question = activePoll.Question;
                else
                    voteIndex->pollCache.insert(make_pair(activePoll.ID, activePoll));
            } else if (strType.find("Local", typeLen) != string::npos)
            {
                if (stack.pIt != voteIndex->pollStack.end())
                    stack.poll->Question = activePoll.Question;
                else
                    voteIndex->pollStack.insert(make_pair(activePoll.ID, activePoll));
            }
        }
        else if (strType.rfind("address", 0) == 0)
        {
            ssValue >> activePoll.strAddress;

            size_t typeLen = strlen("address");
            if (strType.size() == typeLen)
            {
                if (stack.pIt != voteIndex->pollCache.end())
                    stack.poll->strAddress = activePoll.strAddress;
                else
                    voteIndex->pollCache.insert(make_pair(activePoll.ID, activePoll));
            } else if (strType.find("Local", typeLen) != string::npos)
            {
                if (stack.pIt != voteIndex->pollStack.end())
                    stack.poll->strAddress = activePoll.strAddress;
                else
                    voteIndex->pollStack.insert(make_pair(activePoll.ID, activePoll));
            }
        }
        else if (strType.rfind("option", 0) == 0)
        {
            string strPollOption;
            ssValue >> strPollOption;
            activePoll.Option.push_back(strPollOption);

            size_t typeLen = strlen("option#");
            if (strType.size() == typeLen)
            {
                if (stack.pIt != voteIndex->pollCache.end())
                {
                    stack.poll->Option.push_back(strPollOption);
                    stack.poll->OpCount = (uint8_t)stack.poll->Option.size();
                }
                else
                    voteIndex->pollCache.insert(make_pair(activePoll.ID, activePoll));

            } else if (strType.find("Local", typeLen) != string::npos)
            {
                if (stack.pIt != voteIndex->pollStack.end())
                {
                    stack.poll->Option.push_back(strPollOption);
                    stack.poll->OpCount = (uint8_t)stack.poll->Option.size();
                }
                else
                    voteIndex->pollStack.insert(make_pair(activePoll.ID, activePoll));
            }
        }
        else if (strType == "txhash")
        {
            string strTxHash;
            ssValue >> strTxHash;
            activePoll.hash.SetHex(strTxHash);

            size_t typeLen = strlen("txhash");
            if (strType.size() == typeLen)
            {
                if (stack.pIt != voteIndex->pollCache.end())
                    stack.poll->hash = activePoll.hash;
                else
                    voteIndex->pollCache.insert(make_pair(activePoll.ID, activePoll));
            }
        }
        else if (strType == "height")
        {
            string strHeight;
            ssValue >> strHeight;
            activePoll.nHeight = (uint64_t)stoul(strHeight);

            size_t typeLen = strlen("height");
            if (strType.size() == typeLen)
            {
                if (stack.pIt != voteIndex->pollCache.end())
                    stack.poll->nHeight = activePoll.nHeight;
                else
                    voteIndex->pollCache.insert(make_pair(activePoll.ID, activePoll));
            }
        }
        else if (strType.rfind("t", 0) == 0) // never add anything starting with t after this.
        {
            string strTally;
            string strTallyType = strType.substr(1, strType.size() - 2);
            uint8_t nTally = (uint8_t)stoi(strType.substr(strType.size() - 1, 1));
            uint32_t dummyPoint = 0;
            uint32_t *tallyPoint = &dummyPoint;

            ssValue >> strTally;

            bool typeOk = false;
            if (strTallyType == "POS")
                typeOk = true;
            else if (strTallyType == "FPOS")
                typeOk = true;
            else if (strTallyType == "POW")
                typeOk = true;
            else if (strTallyType == "D4L")
                typeOk = true;

            if (!typeOk)
                return false;

            if (stack.pIt == voteIndex->pollCache.end())
            {
                voteIndex->pollCache.insert(make_pair(activePoll.ID, activePoll));
                stack.pIt = voteIndex->pollCache.find(activePoll.ID);
                stack.setActive(stack.pIt, stack.bIt, ActivePoll::SET_POLL);
            }

            if(stack.poll->nTally.size() < nTally)
                stack.poll->nTally.resize(nTally);  // Should only have to do this once as we write top first.

            if (strTallyType == "POS")
                tallyPoint = &stack.poll->nTally[nTally-1].POS;
            else if (strTallyType == "FPOS")
                tallyPoint = &stack.poll->nTally[nTally-1].FPOS;
            else if (strTallyType == "POW")
                tallyPoint = &stack.poll->nTally[nTally-1].POW;
            else if (strTallyType == "D4L")
                tallyPoint = &stack.poll->nTally[nTally-1].D4L;

            *tallyPoint = (uint32_t)stoul(strTally);

        }
        else if (strType == "ballotID")
        {
            string strOpSelection;
            ssValue >> strOpSelection;
            activeBallot.OpSelection = (COptionID)stoi(strOpSelection);
            activeBallot.PollID = activePoll.ID;

            if (stack.bIt == voteIndex->ballotStack.end() )
                voteIndex->ballotStack.insert(make_pair(activePoll.ID, activeBallot));
        }
    } catch (...)
    {
        return false;
    }


    return true;

}

VDBErrors CVoteDB::LoadVote(CVote* voteIndex)
{
    bool fNoncriticalErrors = false;
    VDBErrors result = VDB_LOAD_OK;

    try {
        LOCK(voteIndex->cs_wallet);

        // Get cursor
        Dbc* pcursor = GetCursor();
        if (!pcursor)
        {
            printf("Error getting wallet database cursor\n");
            return VDB_CORRUPT;
        }

        while (true)
        {
            // Read next record
            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            int ret = ReadAtCursor(pcursor, ssKey, ssValue);
            if (ret == DB_NOTFOUND)
                break;
            else if (ret != 0)
            {
                printf("Error reading next record from wallet database\n");
                return VDB_CORRUPT;
            }

            // Try to be tolerant of single corrupt records:
            string strType, strErr;
            if (!ReadElementValue(voteIndex, ssKey, ssValue, strType, strErr))
            {
                printf("\n\nError: Debug ReadKeyValue for VoteDB \n\n");
            }
            if (!strErr.empty())
                printf("%s\n", strErr.c_str());

        }
        for (PollStack::iterator sIt = voteIndex->pollStack.begin(); sIt != voteIndex->pollStack.end(); sIt++)
        {
            if (sIt->second.Option.size() != sIt->second.nTally.size())
                sIt->second.nTally.resize(sIt->second.Option.size());

            PollStack::iterator pIt = voteIndex->pollCache.find(sIt->second.ID);
            if (pIt != voteIndex->pollCache.end() && sIt->second.ID != 0)
            {
                sIt->second.hash = pIt->second.hash;
                sIt->second.nHeight = pIt->second.nHeight;
                sIt->second.strAddress = pIt->second.strAddress;
                for(int c = 0; c < pIt->second.OpCount; c++)
                {
                    sIt->second.nTally[c].POS = pIt->second.nTally[c].POS;
                    sIt->second.nTally[c].FPOS = pIt->second.nTally[c].FPOS;
                    sIt->second.nTally[c].POW = pIt->second.nTally[c].POW;
                    sIt->second.nTally[c].D4L = pIt->second.nTally[c].D4L;
                }
                pIt++;
            }

        }
        pcursor->close();
    }
    catch (...)
    {
        result = VDB_CORRUPT;
    }

    if (fNoncriticalErrors && result == VDB_LOAD_OK)
        result = VDB_NONCRITICAL_ERROR;

    // Any Votedb corruption at all: skip any rewriting or
    // upgrading, we don't want to make it worse.
    if (result != VDB_LOAD_OK)
        return result;

    return result;
}

void ThreadFlushVoteDB(void* parg)
{
    // Make this thread recognisable as the votes flushing thread
    RenameThread("pinkcoin-votes");

    const string& strFile = ((const string*)parg)[0];
    static bool fOneThread;
    if (fOneThread)
        return;
    fOneThread = true;
    if (!GetBoolArg("-flushvotes", true))
        return;

    unsigned int nLastSeen = nVoteDBUpdated;
    unsigned int nLastFlushed = nVoteDBUpdated;
    int64_t nLastVoteUpdate = GetTime();
    while (!fShutdown)
    {
        MilliSleep(349);  // Lets do this on a prime to limit collisions with other DB's.

        if (nLastSeen != nVoteDBUpdated)
        {
            nLastSeen = nVoteDBUpdated;
            nLastVoteUpdate = GetTime();
        }

        if (nLastFlushed != nVoteDBUpdated && GetTime() - nLastVoteUpdate >= 1)
        {
            TRY_LOCK(bitdb.cs_db,lockDb);
            if (lockDb)
            {
                // Don't do this if any databases are in use
                int nRefCount = 0;
                map<string, int>::iterator mi = bitdb.mapFileUseCount.begin();
                while (mi != bitdb.mapFileUseCount.end())
                {
                    nRefCount += (*mi).second;
                    mi++;
                }

                if (nRefCount == 0 && !fShutdown)
                {
                    map<string, int>::iterator mi = bitdb.mapFileUseCount.find(strFile);
                    if (mi != bitdb.mapFileUseCount.end())
                    {
                        printf("Flushing Vote.dat\n");
                        nLastFlushed = nVoteDBUpdated;
                        int64_t nStart = GetTimeMillis();

                        // Flush Vote.dat so it's self contained
                        bitdb.CloseDb(strFile);
                        bitdb.CheckpointLSN(strFile);

                        bitdb.mapFileUseCount.erase(mi++);
                        printf("Flushed Vote.dat %" PRId64 "ms\n", GetTimeMillis() - nStart);
                    }
                }
            }
        }
    }
}

/*
bool BackupVoteDB(const CWallet& VoteDB, const string& strDest)
{
    if (!VoteDB.fFileBacked)
        return false;
    while (!fShutdown)
    {
        {
            LOCK(bitdb.cs_db);
            if (!bitdb.mapFileUseCount.count(VoteDB.strWalletFile) || bitdb.mapFileUseCount[VoteDB.strWalletFile] == 0)
            {
                // Flush log data to the dat file
                bitdb.CloseDb(VoteDB.strWalletFile);
                bitdb.CheckpointLSN(VoteDB.strWalletFile);
                bitdb.mapFileUseCount.erase(VoteDB.strWalletFile);

                // Copy wallet.dat
                filesystem::path pathSrc = GetDataDir() / VoteDB.strWalletFile;
                filesystem::path pathDest(strDest);
                if (filesystem::is_directory(pathDest))
                    pathDest /= VoteDB.strWalletFile;

                try {
#if BOOST_VERSION >= 104000
                    filesystem::copy_file(pathSrc, pathDest, filesystem::copy_option::overwrite_if_exists);
#else
                    filesystem::copy_file(pathSrc, pathDest);
#endif
                    printf("copied wallet.dat to %s\n", pathDest.string().c_str());
                    return true;
                } catch(const filesystem::filesystem_error &e) {
                    printf("error copying wallet.dat to %s - %s\n", pathDest.string().c_str(), e.what());
                    return false;
                }
            }
        }
        MilliSleep(100);
    }
    return false;
}


*/
/*

//
// Try to (very carefully!) recover Vote.dat if there is a problem.
//
bool CVoteDB::Recover(CDBEnv& dbenv, std::string filename, bool fOnlyKeys)
{
    // Recovery procedure:
    // move Vote.dat to Vote.timestamp.bak
    // Call Salvage with fAggressive=true to
    // get as much data as possible.
    // Rewrite salvaged data to wallet.dat
    // Set -rescan so any missing transactions will be
    // found.
    int64_t now = GetTime();
    std::string newFilename = strprintf("Vote.%" PRId64 ".bak", now);

    int result = dbenv.dbenv.dbrename(NULL, filename.c_str(), NULL,
                                      newFilename.c_str(), DB_AUTO_COMMIT);
    if (result == 0)
        printf("Renamed %s to %s\n", filename.c_str(), newFilename.c_str());
    else
    {
        printf("Failed to rename %s to %s\n", filename.c_str(), newFilename.c_str());
        return false;
    }

    std::vector<CDBEnv::KeyValPair> salvagedData;
    bool allOK = dbenv.Salvage(newFilename, true, salvagedData);
    if (salvagedData.empty())
    {
        printf("Salvage(aggressive) found no records in %s.\n", newFilename.c_str());
        return false;
    }
    printf("Salvage(aggressive) found %" PRIszu " records\n", salvagedData.size());

    bool fSuccess = allOK;
    Db* pdbCopy = new Db(&dbenv.dbenv, 0);
    int ret = pdbCopy->open(NULL,                 // Txn pointer
                            filename.c_str(),   // Filename
                            "main",    // Logical db name
                            DB_BTREE,  // Database type
                            DB_CREATE,    // Flags
                            0);
    if (ret > 0)
    {
        printf("Cannot create database file %s\n", filename.c_str());
        return false;
    }
    CWallet dummyVoteDB;

    DbTxn* ptxn = dbenv.TxnBegin();
    BOOST_FOREACH(CDBEnv::KeyValPair& row, salvagedData)
    {
        if (fOnlyKeys)
        {
            CDataStream ssKey(row.first, SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(row.second, SER_DISK, CLIENT_VERSION);
            string strType, strErr;
            bool fReadOK = ReadKeyValue(&dummyVoteDB, ssKey, ssValue, strType, strErr);
            if (!fReadOK)
            {
                printf("WARNING: CVoteDB::Recover skipping %s: %s\n", strType.c_str(), strErr.c_str());
                continue;
            }
        }
        Dbt datKey(&row.first[0], row.first.size());
        Dbt datValue(&row.second[0], row.second.size());
        int ret2 = pdbCopy->put(ptxn, &datKey, &datValue, DB_NOOVERWRITE);
        if (ret2 > 0)
            fSuccess = false;
    }
    ptxn->commit(0);
    pdbCopy->close(0);
    delete pdbCopy;

    return fSuccess;
}
*/
bool CVoteDB::Recover(CDBEnv& dbenv, std::string filename)
{
    return false; // CVoteDB::Recover(dbenv, filename, false);
}

