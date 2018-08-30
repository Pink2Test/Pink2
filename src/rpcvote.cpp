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

void BallotInfo(const Array& params, Object& retObj, string& helpText)
{      
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 != "")
        helpText = ("vote ballotinfo\n"
                            "Returns the PollID and selected vote for the current active poll.\n");

    if (vIndex->current.ballot->PollID != 0)
    {
        try
        {
            retObj.push_back(Pair("PollID", to_string(vIndex->current.ballot->PollID)));
            retObj.push_back(Pair("Selectoin", vIndex->current.poll->Option[vIndex->current.ballot->OpSelection]));
        } catch (...) {
            throw runtime_error("Failed to retrieve ballot info.\n");
        }
    }
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

            int64_t pollStart = GetPollTime(vIndex->current.poll->Start);
            int64_t pollEnd = GetPollTime(vIndex->current.poll->End);

            retObj.push_back(Pair("Poll Start", to_string(pollStart)));
            retObj.push_back(Pair("Poll End", to_string(pollEnd)));

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
            if (vIndex->current.poll->Flags & CVote::POLL_VOTE_PER_ADDRESS)
                retFlags.push_back(Pair("PER_ADDRESS", "enabled"));
            if (vIndex->current.poll->Flags == CVote::POll_FUNDRAISER)
                retFlags.push_back(Pair("FUNDRAISER", "enabled"));
            if (vIndex->current.poll->Flags == CVote::POLL_BOUNTY)
                retFlags.push_back(Pair("BOUNTY", "enabled"));
            if (vIndex->current.poll->Flags == CVote::POLL_CLAIM)
                retFlags.push_back(Pair("CLAIM", "enabled"));

            retObj.push_back(Pair("Flags", retFlags));

            if (vIndex->current.poll->OpCount != (uint8_t)vIndex->current.poll->Option.size())
                throw runtime_error("Number of options does not match opcount.");

            for (int i = vIndex->current.poll->OpCount; i > 0 ; i-- )
            {
                string opIndex = "Option #" + to_string(i);
                retObj.push_back(Pair(opIndex, vIndex->current.poll->Option[i-1]));
            };

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
        retObj.push_back(Pair("coins", numCoins));

    retObj.push_back(Pair("castvote", "WIP"));
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


    retObj.push_back(Pair("Active PollID", to_string(vIndex->current.poll->ID)));
}

void MakeSelection(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    int nSelect = stoi(param1);
    string checkSize = to_string(nSelect);

    if (param1 != checkSize || nSelect > 6 || params.size() > 2)
        helpText = ("vote makeselection <selection number>\n"
                            "Sets the selected option on your ballot for the currently active poll.\n");

    if (nSelect > 0 && nSelect < vIndex->current.poll->OpCount && vIndex->current.ballot->PollID == vIndex->current.poll->ID)
    {
       vIndex->current.ballot->OpSelection = nSelect - 1;
       string opSelection = "Option #" + to_string(nSelect);
       retObj.push_back(Pair("PollID", to_string(vIndex->current.ballot->PollID)));
       retObj.push_back(Pair(opSelection, vIndex->current.poll->Option[vIndex->current.ballot->OpSelection]));

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
                            "Creates a new poll, sets it to active, and returns it's PollID.\n");

    CVotePoll *newPoll = new CVotePoll();
    newPoll->clear();
    vIndex->newPoll(newPoll, true);
    retObj.push_back(Pair("New PollID", to_string(vIndex->current.poll->ID)));

    pvoteDB->WriteVote(*vIndex->current.poll, true);
    pvoteDB->WriteBallot(*vIndex->current.ballot);
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
    if (helpText == "" && vIndex->setPoll(pollID))
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

    retObj.push_back(Pair("submitpoll", "WIP"));
}

void PollName(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote pollname [name] \n"
                            "Sets the name for the currently active poll.\n");

    if (!HaveActive())
        throw runtime_error("There is no currently active poll.\n");

    if (helpText == "" && param1.size() < 16)
    {
        vIndex->current.poll->Name = param1;
        retObj.push_back(Pair("PollID", to_string(vIndex->current.poll->ID)));
        retObj.push_back(Pair("Name", vIndex->current.poll->Name));

        pvoteDB->WriteVote(*vIndex->current.poll, true);
        pvoteDB->WriteBallot(*vIndex->current.ballot);
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

    retObj.push_back(Pair("pollquestion", "WIP"));

}

void PollStart(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote pollstart [unix timestamp] \n"
                            "Sets the start time for the currently active poll.\n");

    retObj.push_back(Pair("pollstart", "WIP"));

}

void PollEnd(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote pollend [unix timestamp] \n"
                            "Sets the end time for the currently active poll.\n");

    retObj.push_back(Pair("pollend", "WIP"));

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
                        "|  PER_ADDRESS - Accept one vote per address.\n"
                        "|  FUNDRAISER  - Designates Poll as Fundraiser. Resets all fields.\n"
                        "|  BOUNTY      - Designates Poll as Bounty Fundraiser. Resets all fields.\n"
                        "|  CLAIM       - Designates Poll as a Fundraiser Claim poll.\n"
                        "|                  Force Sets appropriate flags and resets all fields.\n");


    retObj.push_back(Pair("setflag", "WIP"));

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
                        "|  PER_ADDRESS - Accept one vote per address.\n"
                        "|  FUNDRAISER  - Designates Poll as Fundraiser. Resets all fields.\n"
                        "|  BOUNTY      - Designates Poll as Bounty Fundraiser. Resets all fields.\n"
                        "|  CLAIM       - Designates Poll as a Fundraiser Claim poll.\n"
                        "|                  Force Sets appropriate flags and resets all fields.\n");

    retObj.push_back(Pair("unsetflag", "WIP"));

}

void AddOption(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote addoption [option text] \n"
                            "Adds the poll matching PollID to your locally saved polls to vote on later.\n");

    retObj.push_back(Pair("add", "WIP"));

}

void RemoveOption(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote removeoption [option number] \n"
                            "Adds the poll matching PollID to your locally saved polls to vote on later.\n");

    retObj.push_back(Pair("add", "WIP"));

}

void FundAddress(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote removeoption [option number] \n"
                            "Adds the poll matching PollID to your locally saved polls to vote on later.\n");

    retObj.push_back(Pair("add", "WIP"));

}

void BountyAddress(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 == "help" || params.size() > 2)
        helpText = ("vote removeoption [option number] \n"
                            "Adds the poll matching PollID to your locally saved polls to vote on later.\n");

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

    if (param1 != "help" || params.size() > 3)
        helpText = ("vote searchname \"<search text>\" [page number]\n"
                            "Searches the names of all polls and returns their PollID and full name.\n");

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

    if (param1 != "help" || params.size() > 3)
        helpText = ("vote searchname \"<search text>\" [page number]\n"
                            "Searches the names of all polls and returns their PollID, full name, and question.\n");

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

    retObj.push_back(Pair("add", "WIP"));

}

void ListLocal(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    int nPage = stoi(param1);
    string checkSize = to_string(nPage);

    if (param1 != checkSize || params.size() > 2)
        helpText = ("vote listlocal [page number]\n"
                            "Lists the PollID's, Names, and Ending Times of all polls that you have saved locally off-chain.\n");

    retObj.push_back(Pair("listlocal", "WIP"));
}

void RemovePoll(const Array& params, Object& retObj, string& helpText)
{
    string param1 = "";
    if (params.size() > 1)
        param1 = params[1].get_str();

    if (param1 != "help" || params.size() > 2)
        helpText = ("vote remove <PollID> \n"
                            "Removes the poll matching PollID from your locally saved polls.\n");

    retObj.push_back(Pair("removepoll", "WIP"));
}

Value vote(const Array& params, bool fHelp)
{
    string cHelpText = "";
    string helpText =("vote [command] [params]\n"
                      "Use poll [command] help for command specific information.\n\n"
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
                      "|  bountyaddress [pink address]\n"
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
        BountyAddress(params, retObj, cHelpText);
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

/*
Value getpollinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getvoteinfo [params]\n"
            "Returns current vote information:\n"
            "  \"voteinfo\" : placeholder.\n");

    std::string strMode = "template";
    if (params.size() > 0)
    {
        const Object& oparam = params[0].get_obj();
        const Value& modeval = find_value(oparam, "mode");
        if (modeval.type() == str_type)
            strMode = modeval.get_str();
        else if (modeval.type() == null_type)
        {
            // Do nothing
        }
        else
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");
    }

    if (strMode != "template")
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");

    if (vNodes.empty())
        throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Pinkcoin is not connected!");

    if (IsInitialBlockDownload())
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Pinkcoin is downloading blocks...");

    Object aux;
    aux.push_back(Pair("flags", HexStr(COINBASE_FLAGS.begin(), COINBASE_FLAGS.end())));

    Object result;
    result.push_back(Pair("voteinfo", aux));

    return result;
}
*/
