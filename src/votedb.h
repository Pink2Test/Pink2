// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_VOTEDB_H
#define BITCOIN_VOTEDB_H


#include "db.h"

/** Error statuses for the Vote database */
enum VDBErrors
{
    VDB_LOAD_OK,
    VDB_CORRUPT,
    VDB_NONCRITICAL_ERROR,
    VDB_TOO_NEW,
    VDB_LOAD_FAIL,
    VDB_NEED_REWRITE
};

/** Access to the wallet database (Vote.dat) */
class CVoteDB : public CDB
{
public:
    CVoteDB(std::string strFilename, const char* pszMode="r+") : CDB(strFilename.c_str(), pszMode)
    {
    }
private:
    CVoteDB(const CVoteDB&);
    void operator=(const CVoteDB&);
public:
    Dbc* GetAtCursor()
    {
        return GetCursor();
    }
    
    Dbc* GetVoteCursor()
    {
        if (!pdb)
            return NULL;
        
        DbTxn* ptxnid = activeTxn; // call TxnBegin first
        
        Dbc* pcursor = NULL;
        int ret = pdb->cursor(ptxnid, &pcursor, 0);
        if (ret != 0)
            return NULL;
        return pcursor;
    }
    
    DbTxn* GetAtActiveTxn()
    {
        return activeTxn;
    }
    
    bool WriteVote(const CVotePoll& votePoll, bool isLocal = true);
    bool EraseVote(const CVotePoll& votePoll, bool isLocal = true);
    bool ReadVote(const CVotePoll& votePollID, CVotePoll& votePoll, bool isLocal = true);

    bool WriteBallot(const CVoteBallot& voteBallot);
    bool EraseBallot(const CVoteBallot& voteBallot);
    bool ReadBallot(const CVoteBallot& vBallot, CVoteBallot& voteBallot);

    bool WriteMinVersion(int nVersion)
    {
        return Write(std::string("minversion"), nVersion);
    }
//    VDBErrors ReorderTransactions(CWallet*);C
    VDBErrors LoadVote(CVote *voteIndex);
//    static bool RefreshFromChain(CDBenv& dbenv);
//    static bool Recover(CDBEnv& dbenv, std::string filename, bool fOnlyKeys);
    static bool Recover(CDBEnv& dbenv, std::string filename);
};

#endif // BITCOIN_VOTEDB_H
