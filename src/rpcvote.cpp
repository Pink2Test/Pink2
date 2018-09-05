// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoinrpc.h"
#include "init.h"
#include "votedb.h"
#include "vote.h"
#include "base58.h"
#include <boost/lexical_cast.hpp>

using namespace json_spirit;
using namespace std;
using namespace boost;

bool HaveActive()
{
    if (vIndex->current.poll->ID != 0)
        return true;
    return false;
}

bool isNumber(const string& s)
{
    for (string::const_iterator i = s.begin(); i != s.end(); i++)
    {
        if (isdigit(*i))
            return true;
    }
    return false;
}

void SetTime (string& setTime, CPollTime& pollTime)
{
    pollTime = GetPollTime2(stol(setTime));

    struct tm* timePoint;
    char dT [30];

    time_t sTime = (time_t)GetPollTime(pollTime);
    timePoint = gmtime (&sTime);

    strftime(dT, 30, "%X on %x UTC", timePoint);
    string timeDay(dT);

    setTime = timeDay;
}

void BallotInfo(const Array& params, Object& retObj, string& helpText)
{      
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 != "")
        helpText = ("vote ballotinfo\n"
                            "Returns the PollID and selected vote for the current active poll.\n");

    if (helpText != "")
        return;

    if (vIndex->current.ballot->PollID != 0)
    {
        try
        {
            retObj.push_back(Pair("PollID", to_string(vIndex->current.ballot->PollID)));
            retObj.push_back(Pair("Selection", vIndex->current.poll->Option[vIndex->current.ballot->OpSelection]));
        } catch (...) {
            throw runtime_error("Failed to retrieve ballot info.\n");
        }
    }
}

void PrintFlags(Object& retObj)
{
    if (!HaveActive())
        return;

    Object retFlags;
    if (vIndex->current.poll->Flags == CVote::POLL_ENFORCE_POS)
        retFlags.push_back(Pair("ENFORCE_POS", "enabled"));
    if (vIndex->current.poll->Flags & CVote::POLL_ALLOW_POS)
        retFlags.push_back(Pair("POS", "enabled"));
    if (vIndex->current.poll->Flags & CVote::POLL_ALLOW_FPOS)
        retFlags.push_back(Pair("FPOS", "enabled"));
    if (vIndex->current.poll->Flags & CVote::POLL_ALLOW_POW)
        retFlags.push_back(Pair("POW", "enabled"));
    if (vIndex->current.poll->Flags & CVote::POLL_ALLOW_D4L)
        retFlags.push_back(Pair("D4L", "enabled"));
    if (vIndex->current.poll->Flags & CVote::POLL_PAY_TO_POLL)
        retFlags.push_back(Pair("P2POLL", "enabled"));
    if (vIndex->current.poll->Flags == CVote::POLL_FUNDRAISER)
        retFlags.push_back(Pair("FUNDRAISER", "enabled"));
    if (vIndex->current.poll->Flags == CVote::POLL_BOUNTY)
        retFlags.push_back(Pair("BOUNTY", "enabled"));
    if (vIndex->current.poll->Flags == CVote::POLL_CLAIM)
        retFlags.push_back(Pair("CLAIM", "enabled"));

    retObj.push_back(Pair("Flags", retFlags));
}

void PrintOptions(Object& retObj)
{
    if (vIndex->current.poll->OpCount != vIndex->current.poll->Option.size() || !HaveActive())
        return;

    Object retOps;

    for (int i = 0; i < (int)vIndex->current.poll->OpCount ; i++ )
    {
        string opIndex = "Option #" + to_string(i+1);
        retOps.push_back(Pair(opIndex, vIndex->current.poll->Option[i]));
    };

    retObj.push_back(Pair("Options", retOps));
}

void GetPoll(Object& retObj)
{
    if (vIndex->current.poll->ID != 0)
    {
        try
        {
            retObj.push_back(Pair("PollID", to_string(vIndex->current.poll->ID)));
            retObj.push_back(Pair("Poll Name", vIndex->current.poll->Name));
            retObj.push_back(Pair("Poll Question", vIndex->current.poll->Question));

            string sPollStart = to_string(GetPollTime(vIndex->current.poll->Start));
            string sPollEnd = to_string(GetPollTime(vIndex->current.poll->End));
            CPollTime dummy;

            SetTime(sPollStart, dummy);
            SetTime(sPollEnd, dummy);

            retObj.push_back(Pair("Poll Start", sPollStart));
            retObj.push_back(Pair("Poll End", sPollEnd));

            if (vIndex->current.poll->OpCount != (uint8_t)vIndex->current.poll->Option.size())
                throw runtime_error("Number of options does not match opcount.");

            PrintFlags(retObj);
            PrintOptions(retObj);

        } catch (...) {
            throw runtime_error("Failed to retrieve poll info. \n");
        }
    }
}

void PollInfo(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 != "")
        helpText = ("vote pollinfo\n"
                            "Returns the voting information for the current active poll.\n");

    if (helpText != "")
        return;

    GetPoll(retObj);
}

void Tally(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 != "")
        helpText = ("vote tally\n"
                            "Returns the network voting tally for the currently selected poll.\n");

    if (helpText != "")
        return;

    retObj.push_back(Pair("tally", "WIP"));
}

void CastVote(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    int numCoins = stoi(param1);
    string checkSize = to_string(numCoins);

    if (param1 != checkSize || params.size() > 2)
        helpText = ("vote cast [# of Pink to contribute if required by poll]\n"
                            "Prepares the current ballot and checks that all requirements are\n"
                            "met in order to be cast to the network.\n");

    if (param1 != "")
        return;

    retObj.push_back(Pair("coins", numCoins));
}

void Confirm(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 != "")
        helpText = ("vote confirm\n"
                            "Confirms the currently prepared ballot/poll is complete and accurate, \n"
                            "then submits it and/or registers it to be submitted to the network,\n"
                            "depending on the poll voting requirments and the method being used. \n");

    if (helpText != "")
        return;

    retObj.push_back(Pair("confirm", "WIP"));
}

void GetActive(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 != "")
        helpText = ("vote getactive\n"
                            "Returns the PollID for the currently active poll.\n");

    if (helpText != "")
        return;

    retObj.push_back(Pair("Active PollID", to_string(vIndex->current.poll->ID)));
}

void MakeSelection(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    int nSelect = stoi(param1);
    string checkSize = to_string(nSelect);

    if (param1 != checkSize || nSelect > (int)POLL_OPTION_COUNT || params.size() > 2)
        helpText = ("vote makeselection <selection number>\n"
                            "Sets the selected option on your ballot for the currently active poll.\n");

    if (helpText != "")
        return;

    if (nSelect > 0 && nSelect < (vIndex->current.poll->OpCount +1) && vIndex->current.ballot->PollID == vIndex->current.poll->ID)
    {
       vIndex->current.ballot->OpSelection = nSelect - 1;
       string opSelection = "Option #" + to_string(nSelect);
       retObj.push_back(Pair("PollID", to_string(vIndex->current.ballot->PollID)));
       retObj.push_back(Pair(opSelection, vIndex->current.poll->Option[vIndex->current.ballot->OpSelection]));

       CVoteDB(vIndex->strWalletFile).WriteBallot(*vIndex->current.ballot);
    } else {
        throw runtime_error("Selection out of range.\n");
    }


}

void NewPoll(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 != "")
        helpText = ("vote newpoll\n"
                            "Creates a new poll, sets it to active, and returns its PollID.\n");

    if (helpText != "")
        return;

    CVotePoll *newPoll = new CVotePoll();
    newPoll->clear();
    if (!vIndex->newPoll(newPoll))
        throw runtime_error("New Poll Creation Failed.\n");

    retObj.push_back(Pair("New PollID", to_string(vIndex->current.poll->ID)));

    CVoteDB(vIndex->strWalletFile).WriteVote(*vIndex->current.poll);
    CVoteDB(vIndex->strWalletFile).WriteBallot(*vIndex->current.ballot);
}

void SetActive(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    CPollID pollID = (CPollID)stoul(param1);

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote setactive [PollID]\n"
                            "Sets the currently active poll.\n");
    if (helpText != "")
        return;

    if (vIndex->setPoll(pollID))
    {
        retObj.push_back(Pair("Success", "true"));
        retObj.push_back(Pair("PollID", to_string(vIndex->current.poll->ID)));
        retObj.push_back(Pair("Name", vIndex->current.poll->Name));
    } else {
        retObj.push_back(Pair("Success", "false"));
    }
}

void SubmitPoll(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 != "")
        helpText = ("vote submitpoll\n"
                            "Checks current active poll for completeness, verifies that it is not currently in the chain,\n"
                            "and displays it's current information for user verification.\n\n"
                            "User MUST vote confirm if the information is accurate in order to register \n"
                            "the poll with the blockchain.\n");

    if (helpText != "")
        return;

    // Rawpoll prototype encode/decode for platform testing. Not the final version.
    bool success = false;
    vector<CRawPoll> rawPoll;
    if (getRawPoll(rawPoll))
    {
        if(processRawPoll(rawPoll, vIndex->current.poll->hash, vIndex->current.poll->nHeight, false))
            success = true;
        else
            success = false;
    }

    if (success)
    {
        retObj.push_back(Pair("PollID", to_string(vIndex->current.poll->ID)));
        retObj.push_back(Pair("Submitted", "true"));
    }
    else
    {
        retObj.push_back(Pair("PollID", to_string(vIndex->current.poll->ID)));
        retObj.push_back(Pair("Submitted", "false"));
    }

}

void PollName(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote pollname [name] \n"
                            "Sets the name for the currently active poll.\n");

    if (helpText != "")
        return;

    if (!HaveActive())
        throw runtime_error("There is no currently active poll.\n");

    if (helpText == "" && param1.size() <= POLL_NAME_SIZE)
    {
        vIndex->current.poll->Name = param1;
        retObj.push_back(Pair("PollID", to_string(vIndex->current.poll->ID)));
        retObj.push_back(Pair("Name", vIndex->current.poll->Name));

        CVoteDB(vIndex->strWalletFile).WriteVote(*vIndex->current.poll);
        CVoteDB(vIndex->strWalletFile).WriteBallot(*vIndex->current.ballot);
        //pvoteDB->WriteVote(*vIndex->current.poll, true);
        //pvoteDB->WriteBallot(*vIndex->current.ballot);
    }

}

void PollQuestion(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote pollquestion [question text] \n"
                            "Sets the question for the currently active poll.\n");

    if (helpText != "")
        return;

    if (!HaveActive())
        throw runtime_error("There is no currently active poll.\n");

    if (param1.size() <= POLL_QUESTION_SIZE)
    {
        vIndex->current.poll->Question = param1;
        retObj.push_back(Pair("PollID", to_string(vIndex->current.poll->ID)));
        retObj.push_back(Pair("Question", vIndex->current.poll->Question));

        CVoteDB(vIndex->strWalletFile).WriteVote(*vIndex->current.poll);
        //pvoteDB->WriteVote(*vIndex->current.poll, true);
        //pvoteDB->WriteBallot(*vIndex->current.ballot);
    }

}

void PollStart(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote pollstart [unix timestamp] \n"
                            "Sets the start time for the currently active poll.\n");
    if (helpText != "")
        return;

    if (!HaveActive())
        throw runtime_error("There is no currently active poll.\n");

    if (isNumber(param1) && stol(param1) > GetPollTime(0))
    {
        SetTime(param1, vIndex->current.poll->Start);           // Repurposing Param1 here to hold formatted Day & Time.

        retObj.push_back(Pair("PollID", to_string(vIndex->current.poll->ID)));
        retObj.push_back(Pair("Start", param1));

        CVoteDB(vIndex->strWalletFile).WriteVote(*vIndex->current.poll);
    }
}

void PollEnd(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote pollend [unix timestamp] \n"
                            "Sets the end time for the currently active poll.\n");
    if (helpText != "")
        return;

    if (!HaveActive())
        throw runtime_error("There is no currently active poll.\n");

    if (isNumber(param1) && stol(param1) > GetPollTime(0))
    {
        SetTime(param1, vIndex->current.poll->End);           // Repurposing Param1 here to hold formatted Day & Time.

        retObj.push_back(Pair("PollID", to_string(vIndex->current.poll->ID)));
        retObj.push_back(Pair("End", param1));

        CVoteDB(vIndex->strWalletFile).WriteVote(*vIndex->current.poll);
    }

}

bool FlagSetter(const string& flag, CPollFlags& flags, const bool& set)
{
    bool fSet = false;

    if (set)
    {
        if (flag == "ENFORCE_POS")      { flags = CVote::POLL_ENFORCE_POS; fSet = true; }
        else if (flag == "POS")         { flags |= CVote::POLL_ALLOW_POS; fSet = true; }
        else if (flag == "FPOS")        { flags |= CVote::POLL_ALLOW_FPOS; fSet = true; }
        else if (flag == "POW")         { flags |= CVote::POLL_ALLOW_POW; fSet = true; }
        else if (flag == "D4L")         { flags |= CVote::POLL_ALLOW_D4L; fSet = true; }
        else if (flag == "P2POLL") { flags |= CVote::POLL_PAY_TO_POLL; fSet = true; }
        else if (flag == "FUNDRAISER")  { flags = CVote::POLL_FUNDRAISER; fSet = true; }
        else if (flag == "BOUNTY")      { flags = CVote::POLL_BOUNTY; fSet = true; }
        else if (flag == "CLAIM")       { flags = CVote::POLL_CLAIM; fSet = true; }
    } else {
        if (flag == "ENFORCE_POS")      { flags |= CVote::POLL_ALLOW_POS; fSet = true; }
        else if (flag == "POS")         { flags &= ~CVote::POLL_ALLOW_POS; fSet = true; }
        else if (flag == "FPOS")        { flags &= ~CVote::POLL_ALLOW_FPOS; fSet = true; }
        else if (flag == "POW")         { flags &= ~CVote::POLL_ALLOW_POW; fSet = true; }
        else if (flag == "D4L")         { flags &= ~CVote::POLL_ALLOW_D4L; fSet = true; }
        else if (flag == "P2POLL") { flags &= ~CVote::POLL_PAY_TO_POLL; fSet = true; }
        else if (flag == "FUNDRAISER")  { flags &= ~CVote::POLL_FUNDRAISER; fSet = true; }
        else if (flag == "BOUNTY")      { flags &= ~CVote::POLL_BOUNTY; fSet = true; }
        else if (flag == "CLAIM")       { flags &= ~CVote::POLL_CLAIM; fSet = true; }
    }

    return fSet;


}

void resetOptions()
{
    vIndex->current.poll->Option.clear();
    vIndex->current.poll->OpCount = 0;
}

void unsetForced()
{
    uint8_t CLAIM = (1U << 7);
    vIndex->current.poll->Flags &= ~CVote::POLL_FUNDRAISER;
    vIndex->current.poll->Flags &= ~CVote::POLL_BOUNTY;
    vIndex->current.poll->Flags &= ~CLAIM; // clear just the CLAIM flag, leave POS/FPOS/POW if set.
}

void SetFlag(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote setflag [flag] \n"
                        "Sets a particular flag for the currently active poll.\n"
                        "Available flags:\n"
                        "|  ENFORCE_POS - Resets all flags. Only POS votes accepted.\n"
                        "|  POS         - Accept POS Votes.\n"
                        "|  FPOS        - Accept FPOS Votes. \n"
                        "|  POW         - Accept POW Votes.\n"
                        "|  D4L         - Accept D4L Donation Votes.\n"
                        "|  P2POLL      - Funds are collected and released on successful CLAIM poll.\n"
                        "|  FUNDRAISER  - Designates Poll as Fundraiser.\n"
                        "|  BOUNTY      - Designates Poll as Bounty Fundraiser.\n"
                        "|  CLAIM       - Designates Poll as a Fundraiser Claim poll.\n"
                        "|                  Force Sets required voting block flags.\n");

    if (helpText != "")
        return;

    if (!HaveActive() || helpText != "")
        return;

    if (!FlagSetter(param1, vIndex->current.poll->Flags, true))
        helpText = "Flag not found. Check 'vote setflag help' for available flags.\n";
    else if (param1 == "FUNDRAISER" || param1 == "BOUNTY" || param1 == "CLAIM")
        resetOptions();
    else
        unsetForced();

    PrintFlags(retObj);
    CVoteDB(vIndex->strWalletFile).WriteVote(*vIndex->current.poll);
}

void UnsetFlag(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote unsetflag [flag] \n"
                    "Sets a particular flag for the currently active poll.\n"
                        "Available flags:\n"
                        "|  ENFORCE_POS - Resets all flags. Only POS votes accepted.\n"
                        "|  POS         - Accept POS Votes.\n"
                        "|  FPOS        - Accept FPOS Votes. \n"
                        "|  POW         - Accept POW Votes.\n"
                        "|  D4L         - Accept D4L Donation Votes.\n"
                        "|  P2POLL      - Funds are collected and released on successful CLAIM poll.\n"
                        "|  FUNDRAISER  - Designates Poll as Fundraiser.\n"
                        "|  BOUNTY      - Designates Poll as Bounty Fundraiser. P2POLL Required.\n"
                        "|  CLAIM       - Designates Poll as a Fundraiser Claim poll.\n"
                        "|                  Force Sets appropriate flags and resets all fields.\n");

    if (helpText != "")
        return;

    if (!HaveActive() || helpText != "")
        return;

    if (!FlagSetter(param1, vIndex->current.poll->Flags, false))
        helpText = "Flag not found. Check 'vote setflag help' for available flags.\n";

    PrintFlags(retObj);
    CVoteDB(vIndex->strWalletFile).WriteVote(*vIndex->current.poll);
}

void AddOption(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || param1.size() > POLL_OPTION_SIZE || params.size() > 2)
        helpText = ("vote addoption [option text] \n"
                            "Adds the poll matching PollID to your locally saved polls to vote on later.\n");
    if (helpText != "")
        return;

    if (!HaveActive())
        throw runtime_error("There is no currently active poll.\n");

    if (vIndex->current.poll->Option.size() > (POLL_OPTION_COUNT - 1))
        throw runtime_error("Cannot have more than " + to_string(POLL_OPTION_COUNT) + " options. Use vote removeoption to remove one before adding another.\n");

    if (param1 != "")
    {
        vIndex->current.poll->Option.push_back(param1);
        vIndex->current.poll->OpCount = (COptionID)vIndex->current.poll->Option.size();
        CVoteDB(vIndex->strWalletFile).WriteVote(*vIndex->current.poll);
    }

    retObj.push_back(Pair("PollID", to_string(vIndex->current.poll->ID)));
    retObj.push_back(Pair("Name", vIndex->current.poll->Name));
    retObj.push_back(Pair("Question", vIndex->current.poll->Question));

    PrintOptions(retObj);

}

void RemoveOption(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote removeoption [option number] \n"
                            "Adds the poll matching PollID to your locally saved polls to vote on later.\n");

    if (helpText != "")
        return;

    if (!HaveActive())
        throw runtime_error("There is no currently active poll.\n");

    if (!isNumber(param1) || stoi(param1) > vIndex->current.poll->OpCount || stoi(param1) < 1)
        throw runtime_error("Number out of range or you did not enter an option number.\n");

    CVotePoll holdPoll = *vIndex->current.poll;

    vector<CPollOption>::iterator it = vIndex->current.poll->Option.begin();
    it += (stoi(param1) - 1);

    vIndex->current.poll->Option.erase(it);
    vIndex->current.poll->OpCount = (COptionID)vIndex->current.poll->Option.size();
    vIndex->current.ballot->OpSelection = 0;

    CVoteDB(vIndex->strWalletFile).EraseVote(holdPoll);                 // Ugly but cheap overall.
    CVoteDB(vIndex->strWalletFile).WriteBallot(*vIndex->current.ballot);
    CVoteDB(vIndex->strWalletFile).WriteVote(*vIndex->current.poll);

    retObj.push_back(Pair("PollID", to_string(vIndex->current.poll->ID)));
    retObj.push_back(Pair("Name", vIndex->current.poll->Name));
    retObj.push_back(Pair("Question", vIndex->current.poll->Question));

    PrintOptions(retObj);

}

void FundAddress(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote removeoption [option number] \n"
                            "Adds the poll matching PollID to your locally saved polls to vote on later.\n");

    if (helpText != "")
        return;

    retObj.push_back(Pair("add", "WIP"));

}

void ClaimAddress(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote removeoption [option number] \n"
                    "Adds the poll matching PollID to your locally saved polls to vote on later.\n");

    if (helpText != "")
        return;

    retObj.push_back(Pair("add", "WIP"));

}

void ListActive(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    int nPage = stoi(param1);
    string checkSize = to_string(nPage);

    if (param1 != checkSize || params.size() > 2)
        helpText = ("vote listactive [page number]\n"
                            "Lists the PollID's, Names, and Ending Times of all open polls on the network .\n");

    if (helpText != "")
        return;

    retObj.push_back(Pair("listactive", "WIP"));
}

void ListComplete(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    int nPage = stoi(param1);
    string checkSize = to_string(nPage);

    if (param1 != checkSize || params.size() > 2)
        helpText = ("vote listcomplete [page number]\n"
                            "Lists the PollID's, Names, and Ending Times of all completed polls on the network .\n");

    if (helpText != "")
        return;

    retObj.push_back(Pair("listcomplete", "WIP"));
}

void ListUpcoming(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    int nPage = stoi(param1);
    string checkSize = to_string(nPage);

    if (param1 != checkSize || params.size() > 2)
        helpText = ("vote listupcoming [page number]\n"
                            "Lists the PollID's, Names and Starting Times of all upcoming polls on the network .\n");

    if (helpText != "")
        return;

    retObj.push_back(Pair("listupcoming", "WIP"));
}

void SearchName(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    int nPage = 0;
    if (params.size() > 2)
        nPage = stoi(params[2].get_str());

    if (nPage > 0)
        printf("Have Pages\n");

    if (param1 != "help" || params.size() > 3)
        helpText = ("vote searchname \"<search text>\" [page number]\n"
                            "Searches the names of all polls and returns their PollID and full name.\n");

    if (helpText != "")
        return;

    retObj.push_back(Pair("searchname", "WIP"));
}

void SearchQuestion(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    int nPage = 0;
    if (params.size() > 2)
        nPage = stoi(params[2].get_str());

    if (nPage > 0)
        printf("Have Pages\n");

    if (param1 != "help" || params.size() > 3)
        helpText = ("vote searchname \"<search text>\" [page number]\n"
                            "Searches the names of all polls and returns their PollID, full name, and question.\n");

    if (helpText != "")
        return;

    retObj.push_back(Pair("searchdesc", "WIP"));
}

void AddPoll(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 != "help" || params.size() > 2)
        helpText = ("vote add <PollID> \n"
                            "Adds the poll matching PollID to your locally saved polls to vote on later.\n");
    if (helpText != "")
        return;

    retObj.push_back(Pair("add", "WIP"));

}

void ListLocal(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    int nPage = isNumber(param1) ? stoi(param1) : 0;
    param1 = (bool)(param1 == "") ? "0" : param1;

    string checkSize = to_string(nPage);

    if (param1 != checkSize || params.size() > 2)
        helpText = ("vote listlocal [page number]\n"
                            "Lists the PollID's, Names, and Ending Times of all polls that you have saved locally off-chain.\n");

    if (helpText != "") { return; }

    CPollID counter = 0;
    for (PollStack::iterator it = vIndex->pollStack.begin() ; it != vIndex->pollStack.end(); it++)
    {
        if (counter == 0)
        {
            if (nPage == 0) { nPage = 1; }
            retObj.push_back(Pair("PollID", "Name"));
        }

        counter++;
        int thisPage = (counter / 10) + 1;

        if (thisPage == nPage)
        {
            CVotePoll p = it->second;

            if (p.ID != 0)
            {
                retObj.push_back(Pair(to_string(p.ID), p.Name));
            }
        }
    }

    int allPages = bool(counter > 0) ? (counter/10) + 1 : 0;
    string retPage = to_string(nPage) + " of " + to_string(allPages);
    retObj.push_back(Pair("Page", retPage));
}

void RemovePoll(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote remove <PollID> \n"
                            "Removes the poll matching PollID from your locally saved polls.\n");

    if (helpText != "") { return; }

    CPollID pollID = 0;
    if (isNumber(param1))
        pollID = stoul(param1);

    PollStack::iterator it = vIndex->pollStack.find(pollID);

    bool success = false;

    if (it != vIndex->pollStack.end() && it->first != 0)
    {

        CVoteDB voteDB(vIndex->strWalletFile);

        success = voteDB.EraseVote(it->second);

        if (vIndex->ballotStack.find(pollID) != vIndex->ballotStack.end())
            voteDB.EraseBallot(vIndex->ballotStack.at(pollID));

        if (vIndex->current.poll->ID == it->first)
            vIndex->current.setActive(vIndex->current.pIt, vIndex->current.bIt, ActivePoll::SET_CLEAR);
        vIndex->pollStack.erase(it);
        //pvoteDB->EraseVote(it->second);
        //pvoteDB->EraseBallot(vIndex->ballotStack.at(it->first));
    }

    string isSuccess = success ? "true" : "false";
    retObj.push_back(Pair("PollID", to_string(pollID)));
    retObj.push_back(Pair("Removed", isSuccess));
}

Value vote(const Array& params, bool fHelp)
{
    string cHelpText = "";
    string helpText =("vote [command] [params]\n"
                      "Use vote [command] help for command specific information.\n\n"
                      "Available Poll Commands:\n"
                      "Information:\n"
                      "|  ballotinfo\n"
                      "|  pollinfo\n"
                      "|  tally\n"
                      "Active Controls:\n"
                      "|  cast [# of PINK (if required to vote)]\n"
                      "|  confirm\n"
                      "|  getactive\n"
                      "|  makeselection [Poll Option Number]\n"
                      "|  newpoll\n"
                      "|  setactive [PollID]\n"
                      "|  submitpoll\n"
                      "Editing Controls:\n"
                      "|  pollname [name]\n"
                      "|  pollquestion [question text]\n"
                      "|  pollstart [unix timestamp]\n"
                      "|  pollend [unix timestamp]\n"
                      "|  setflag [flag]\n"
                      "|  unsetflag [flag]\n"
                      "|  addoption [option text]\n"
                      "|  removeoption [option number]\n"
                      "|  fundaddress [pink address]\n"
                      "|  claimaddress [pink address]\n"
                      "|  claimpoll [pollID]\n"
                      "Navigation:\n"
                      "|  listactive [page number]\n"
                      "|  listcomplete [page number]\n"
                      "|  listupcoming [page number]\n"
                      "|  searchname <search term> [page number]\n"
                      "|  searchquestion <search term> [page number]\n"
                      "Saved Poll Controls:\n"
                      "|  add [PollID]\n"
                      "|  listlocal [page number]\n"
                      "|  remove [PollID]\n");
    if (fHelp || params.size() < 1)
        throw runtime_error(helpText);

    string command = params[0].get_str();
    Object retObj;

    if (command == "ballotinfo")
        BallotInfo(params, retObj, cHelpText);
    else if (command == "pollinfo")
        PollInfo(params, retObj, cHelpText);
    else if (command == "tally")
        Tally(params, retObj, cHelpText);
    else if (command == "cast")
        CastVote(params, retObj, cHelpText);
    else if (command == "confirm")
        Confirm(params, retObj, cHelpText);
    else if (command == "getactive")
        GetActive(params, retObj, cHelpText);
    else if (command == "makeselection")
        MakeSelection(params, retObj, cHelpText);
    else if (command == "newpoll")
        NewPoll(params, retObj, cHelpText);
    else if (command == "setactive")
        SetActive(params, retObj, cHelpText);
    else if (command == "submitpoll")
        SubmitPoll(params, retObj, cHelpText);
    else if (command == "pollname")
        PollName(params, retObj, cHelpText);
    else if (command == "pollquestion")
        PollQuestion(params, retObj, cHelpText);
    else if (command == "pollstart")
        PollStart(params, retObj, cHelpText);
    else if (command == "pollend")
        PollEnd(params, retObj, cHelpText);
    else if (command == "setflag")
        SetFlag(params, retObj, cHelpText);
    else if (command == "unsetflag")
        UnsetFlag(params, retObj, cHelpText);
    else if (command == "addoption")
        AddOption(params, retObj, cHelpText);
    else if (command == "removeoption")
        RemoveOption(params, retObj, cHelpText);
    else if (command == "fundaddress")
        FundAddress(params, retObj, cHelpText);
    else if (command == "bountyaddress")
        ClaimAddress(params, retObj, cHelpText);
    else if (command == "listactive")
        ListActive(params, retObj, cHelpText);
    else if (command == "listcomplete")
        ListComplete(params, retObj, cHelpText);
    else if (command == "listupcoming")
        ListUpcoming(params, retObj, cHelpText);
    else if (command == "searchname")
        SearchName(params, retObj, cHelpText);
    else if (command == "searchquestion")
        SearchQuestion(params, retObj, cHelpText);
    else if (command == "add")
        AddPoll(params, retObj, cHelpText);
    else if (command == "listlocal")
        ListLocal(params, retObj, cHelpText);
    else if (command == "remove")
        RemovePoll(params, retObj, cHelpText);
    else
        throw runtime_error(helpText);

    if (cHelpText != "")
        return (Value)cHelpText;

    return retObj;
}
