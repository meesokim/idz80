/****************************************************************
 * Name:      IDZ80
 * Purpose:   Disassembler for Z80 MSX
 * Author:    Felipe Mainieri (felipe.mpc@gmail.com)
 * Created:   2009-11-09
 * Copyright: Felipe Mainieri ()
 * License:   GPL
 ***
 * This module controls subroutines
 **************************************************************/


#ifndef _SUBROUTINE_H_
#define _SUBROUTINE_H_

#include <stack>
#include <vector>
#include "IDZ80Base.h"
#include "idz80debugbase.h"

struct SubRoutineData
{
    uint    Start,
            Size,
            End;
};

typedef std::vector<SubRoutineData *> SubRoutineList;


class SubRoutineCtrl: public IDZ80LogBase
{
    public:
        bool Call(uint entryaddress, uint nextaddress);
        uint Return(uint exitaddress);
        void UpdateSize();
        bool IsInside();
        bool HasConditionalReturn();
        void SetConditionalReturn();
        void Clear();

        int GetCounter();
        //uint GetAddress();

        SubRoutineCtrl();
        ~SubRoutineCtrl();

    private:
    std::stack<uint>    Stack;
    int                 StackPointer;
    SubRoutineList      SubRoutines;
    SubRoutineData      *ActualSubRoutine;
    SortedIntArray      *CalledSubroutines;
    bool                ReturnConditional;
};


#endif // _SUBROUTINE_H_