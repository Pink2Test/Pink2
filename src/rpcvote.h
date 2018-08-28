#ifndef RPCVOTE_H
#define RPCVOTE_H

#include "votedb.h"
#include "vote.h"
#include "bitcoinrpc.h"
#include "init.h"
#include "base58.h"
#include <boost/lexical_cast.hpp>

using namespace json_spirit;
using namespace std;
using namespace boost;

void BallotInfo(const Value& params, Object& retObj, string& helpText = "");
void PollInfo(const Value& params, Object& retObj, string& helpText = "");
void Tally(const Value& params, Object& retObj, string& helpText = "");
void CastVote(const Value& params, Object& retObj, string& helpText = "");
void Confirm(const Value& params, Object& retObj, string& helpText = "");
void GetActive(const Value& params, Object& retObj, string& helpText = "");
void MakeSelection(const Value& params, Object& retObj, string& helpText = "");
void NewPoll(const Value& params, Object& retObj, string& helpText = "");
void SetActive(const Value& params, Object& retObj, string& helpText = "");
void SubmitPoll(const Value& params, Object& retObj, string& helpText = "");
void ListActive(const Value& params, Object& retObj, string& helpText = "");
void ListComplete(const Value& params, Object& retObj, string& helpText = "");
void ListUpcoming(const Value& params, Object& retObj, string& helpText = "");
void SearchName(const Value& params, Object& retObj, string& helpText = "");
void SearchQuestion(const Value& params, Object& retObj, string& helpText = "");
void AddPoll(const Value& params, Object& retObj, string& helpText = "");
void ListLocal(const Value& params, Object& retObj, string& helpText = "");
void RemovePoll(const Value& params, Object& retObj, string& helpText = "");

#endif // RPCVOTE_H
