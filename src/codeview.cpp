/****************************************************************
 * Name:      IDZ80
 * Purpose:   Defines Application Frame
 * Author:    Felipe Mainieri (felipe.mpc@gmail.com)
 * Created:   2009-11-09
 * Copyright: Felipe Mainieri ()
 * License:   GPL
 *
 * This module shows/controls disassembled data
 **************************************************************/

#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/utils.h>
#include <wx/region.h>
#include <wx/textdlg.h>
#include <wx/dcclient.h>
#include "codeview.h"
#include "d_asm_element.h"



const int CodeView::COL_ADDRESS;
const int CodeView::COL_CODE;
const int CodeView::COL_ASCII;
const int CodeView::COL_LABEL;
const int CodeView::COL_MNEM;



CodeView::CodeView(wxWindow *parent, ProcessData *processparent, LogWindow *logparent)
        : wxScrolledCanvas(parent)
{
    Process = processparent;
    m_linesShown = 0;
    CursorPosition = -1;
    CursorLastPosition = -1;
    LastCursorRect = 0;
    SelectedItemIndex = -1;
    SelectedLastItem = -1;
    SelectedCount = -1;
    MultiSelection = false;
    Selecting = false;
    m_fontHeight = 1;
    IsFocused = false;
    IsEmpty = true;
    m_styleData.arg = 0;
    m_styleData.item = 0;


    m_iteminfo.type = siUnknown;
    m_iteminfo.dasmitem = 0;
    m_iteminfo.lineitem = 0;
    m_iteminfo.hasComment = false;

    m_CodeViewLine = Process->CodeViewLines;
    LastCursorRect = new wxRect();
    IncompleteArea.x = 0;
    IncompleteArea.y = 0;
    IncompleteArea.width = 0;
    IncompleteArea.height = 0;


    SetupFont();

    SetScrollRate(0, m_fontHeight);
    SetVirtualSize(wxDefaultCoord, m_fontHeight);

    SetupColors();

    SetupEvents();


    // Debug Area
    SetTextLog(logparent);
    ModuleName = "CodeView";

}



void CodeView::SetupFont()
{
    wxClientDC dc(this);

    font = new wxFont(9, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL,
            wxFONTWEIGHT_NORMAL, false, "Courier New 10 Pitch");
    dc.SetFont(*font);
    m_fontHeight = dc.GetTextExtent("X").GetHeight();
    m_fontWidth = dc.GetTextExtent("X").GetWidth();
}




void CodeView::SetupColors()
{
    BackGroundColor.Set("WHITE");
    ForeGroundColor.Set("BLACK");
    SelectedItemColor.Set("YELLOW");
    BGArgumentColor.Set("YELLOW");
    FGArgumentColor.Set("GOLD");
    FG_TextColor.Set("BLACK");
    FG_LabelColor.Set("BLUE");
}




void CodeView::SetupEvents()
{
    Bind(wxEVT_SIZE, &CodeView::OnSize, this);
    Bind(wxEVT_SCROLLWIN_LINEDOWN, &CodeView::OnScrollLineDown, this);
    Bind(wxEVT_SCROLLWIN_LINEUP, &CodeView::OnScrollLineUp, this);
    Bind(wxEVT_SCROLLWIN_PAGEDOWN, &CodeView::OnScrollPageDown, this);
    Bind(wxEVT_SCROLLWIN_PAGEUP, &CodeView::OnScrollPageUp, this);


    Bind(wxEVT_PAINT, &CodeView::OnPaint, this);

    Bind(wxEVT_SET_FOCUS, &CodeView::OnGetFocus, this);
    Bind(wxEVT_KILL_FOCUS, &CodeView::OnKillFocus, this);
    Bind(wxEVT_MOUSE_CAPTURE_LOST, &CodeView::OnMouseCaptureLost, this);


    // MOUSE Events
    Bind(wxEVT_LEFT_DOWN, &CodeView::OnMouseLeftDown, this);
    Bind(wxEVT_LEFT_UP, &CodeView::OnMouseLeftUp, this);
    Bind(wxEVT_RIGHT_DOWN, &CodeView::OnMouseRightDown, this);
    Bind(wxEVT_RIGHT_UP, &CodeView::OnMouseRightUp, this);
    //TODO: Improve focus highlight
    Bind(wxEVT_MOTION, &CodeView::OnMouseMove, this);
    Bind(wxEVT_MOUSEWHEEL, &CodeView::OnMouseWheel, this);
    //Bind(wxEVT_ENTER_WINDOW, &CodeView::OnMouseEnterWindow, this);
    //Bind(wxEVT_LEAVE_WINDOW, &CodeView::OnMouseLeaveWindow, this);

    // KEYBOARD events
    Bind(wxEVT_KEY_DOWN, &CodeView::OnKeyPress, this);
    Bind(wxEVT_KEY_UP, &CodeView::OnKeyRelease, this);

    // Popup event connections
    Bind(wxEVT_COMMAND_MENU_SELECTED, &CodeView::OnPopUpAddComment, this, idPOPUP_ADDCOMMENT);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &CodeView::OnPopUpEditComment, this, idPOPUP_EDITCOMMENT);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &CodeView::OnPopUpDelComment, this, idPOPUP_DELCOMMENT);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &CodeView::OnPopUpMenuGoto, this, idPOPUP_GOTO);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &CodeView::OnPopUpMenuGotoAddress, this, idPOPUP_GOTO_ADDRESS);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &CodeView::OnPopUpMenuMakeData, this, idPOPUP_MAKEDATA);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &CodeView::OnPopUpMenuDisasm, this, idPOPUP_DISASM);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &CodeView::OnPopUpMenuOD_Matrix, this, idPOPUP_OD_MATRIX);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &CodeView::OnPopUpMenuOD_String, this, idPOPUP_OD_STRING);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &CodeView::OnPopUpMenuOD_Number, this, idPOPUP_OD_NUMBER);

    Bind(wxEVT_COMMAND_MENU_SELECTED, &CodeView::OnPopUpMenuArgStyleBin, this, idPOPUP_ARG_BIN);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &CodeView::OnPopUpMenuArgStyleDec, this, idPOPUP_ARG_DEC);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &CodeView::OnPopUpMenuArgStyleHex, this, idPOPUP_ARG_HEX);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &CodeView::OnPopUpMenuRenLabel, this, idPOPUP_EDITLABEL);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &CodeView::OnPopUpMenuDelLabel, this, idPOPUP_DELLABEL);
}





int CodeView::GetCount()
{
    return m_CodeViewLine->GetCount();
}




void CodeView::OnDraw(wxDC &dc)
{
    int			fromLine, totalLines, fixedY, fixedHeight;
    wxRegionIterator
				regIterator(GetUpdateRegion());
    wxRect		touchedRegion, fullRect;


    dc.SetFont(*font);
    dc.SetBackground(*wxWHITE_BRUSH);

    // cleans the destroyed region
    while (regIterator)
    {
        touchedRegion = regIterator.GetRect();
        fullRect.Union(touchedRegion);
        regIterator++;
    }
    CalcUnscrolledPosition(fullRect.x, fullRect.y, &fullRect.x, &fullRect.y);

    fullRect.x = 0;
    fromLine = fullRect.y / m_fontHeight;
	fixedY = fromLine * m_fontHeight;
	fixedHeight = fullRect.height + fullRect.y - fixedY;
    totalLines = fixedHeight / m_fontHeight;
    if ((fixedHeight % m_fontHeight) != 0)
        totalLines++;

    fullRect.y = fixedY;
    fullRect.height = totalLines * m_fontHeight;
    fullRect.width = GetClientSize().GetWidth();

	if (IsEmpty)
    {
        PaintBackground(dc, fullRect.y, fromLine, totalLines, *wxGREY_BRUSH);
    }
    else
    {
        if (IsEnabled())
        {
            PaintBackground(dc, fullRect.y, fromLine, totalLines, *wxWHITE_BRUSH);
            if ((fromLine == CursorPosition) || ((CursorPosition >= fromLine) &&
                (CursorPosition < (fromLine + totalLines))))
                ShowCursor(dc);
            Render(dc, fullRect.y, fromLine, totalLines);
        }
        else
        {
            RenderBackGroundBlur(dc, fullRect);
        }
    }
}



void CodeView::RenderBackGroundBlur(wxDC &dc, wxRect region)
{
    wxBitmap window;

    LogIt("Rendering blur...");
    //dc.DestroyClippingRegion();
    LogIt(wxString::Format("Region(%d, %d, %d, %d)", region.x, region.y, region.width, region.height));
    window = DisassembleWindowBitmap.GetSubBitmap(region);
    window = window.ConvertToDisabled(128);
    //*window = ;
    dc.DrawBitmap(window, region.GetPosition());
}


void CodeView::TestBlur()
{
    wxClientDC dc(this);

    RenderBackGroundBlur(dc, GetClientRect());
}


void CodeView::PaintBackground(wxDC &dc, const int start_y, const int fromline, const int toline, const wxBrush backcolour)
{
    int i, y;

    wxSize sz = GetClientSize();

    dc.SetPen(*wxTRANSPARENT_PEN);

    for (i = 0; i < toline; i++)
    {
        if (MultiSelection &&
           ((fromline + i) >= SelectedItemIndex) &&
           ((fromline + i) <= SelectedLastItem))
            dc.SetBrush(wxBrush(SelectedItemColor));
        else
            dc.SetBrush(backcolour);

        y = (i * m_fontHeight) + start_y;
        dc.DrawRectangle(0, y, sz.x, m_fontHeight);
    }
}




void CodeView::OnPaint(wxPaintEvent& event)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    wxAutoBufferedPaintDC dc(this);
    DoPrepareDC(dc);
    UpdateLastCursorRect();
    OnDraw(dc);
    UpdateVirtualSize();
}







CodeView::~CodeView()
{
    delete LastCursorRect;
    delete font;
}




void CodeView::Clear()
{
    ClearSelection();
    IsFocused = false;
    IsEmpty = true;
    if (Process->Disassembled != 0)
        Process->Disassembled->Clear();

    m_CodeViewLine->Clear();
    Refresh();
}



void CodeView::ClearSelection()
{
    SelectedCount = -1;
    SelectedItemIndex = -1;
    SelectedLastItem = -1;
    CursorPosition = -1;
    MultiSelection = false;
    Selecting = false;
}



bool CodeView::Enable(bool enable)
{
    bool ret = false;

    if (enable)
    {
        wxScrolledCanvas::Enable(true);
        SetVirtualSize(wxDefaultCoord, m_CodeViewLine->GetCount() * m_fontHeight);
        IsEmpty = m_CodeViewLine->IsEmpty();
        ret = true;
    }
    else
    {
    	SetVirtualSize(0, 0);
        wxScrolledCanvas::Enable(false);
    }
    return ret;
}

// Receives device coordenate
void CodeView::CalcCursorPosition(wxPoint point)
{
    int cursor;

    CalcUnscrolledPosition(point.x,point.y,&point.x,&point.y);
    cursor = int(point.y / m_fontHeight);

    if (cursor <= GetLastLine())
        CursorPosition = cursor;
}

wxRect CodeView::CalcCursorRfshRect()
{
    wxRect cursor(0, CursorPosition * m_fontHeight, GetClientSize().x, m_fontHeight);
    CalcScrolledPosition(cursor.x, cursor.y, 0, &cursor.y);
    return (cursor);
}



void CodeView::DoSelection()
{
    if (Selecting)
    {
        UpdateSelectedRect();
        if (MultiSelection)
            RefreshRect(CalcSelectedRect());
    }
    else
    {
        if (MultiSelection)
            RefreshRect(CalcSelectedRect());
        SelectedCount = 1;
        MultiSelection = false;
        SelectedItemIndex = CursorPosition;
        CursorLastPosition = CursorPosition;
        SelectedLastItem = SelectedItemIndex;
    }
}


void CodeView::UpdateSelectedRect()
{
    int index;

    if (CursorPosition >= CursorLastPosition)
    {
        index = CursorLastPosition;
        SelectedCount = CursorPosition - CursorLastPosition + 1;
    }
    else
    {
        index = CursorPosition;
        SelectedCount = CursorLastPosition - CursorPosition + 1;
    }
    SelectedItemIndex = index;
    SelectedLastItem = SelectedItemIndex + SelectedCount - 1;
    if (SelectedCount > 1)
        MultiSelection = true;
    else
        MultiSelection = false;
}




// Returns the current Selected items' rectangle
wxRect CodeView::CalcSelectedRect()
{
    wxRect rect;
    int y, count;
    if (SelectedItemIndex < GetFirstLine())
    {
        y = GetFirstLine();
        count = GetFirstLine() - SelectedItemIndex;
    }
    else
    {
        y = SelectedItemIndex;
        count = SelectedCount;
    }
    if ((y + count) >= (GetFirstLine() + m_linesShown))
        count = GetLastLine();
    rect.x = 0;
    rect.y = y * m_fontHeight;
    rect.width = GetClientSize().x;
    rect.height = count * m_fontHeight;
    CalcScrolledPosition(rect.x, rect.y, 0, &rect.y);
    return rect;
}





void CodeView::CalcIncompleteArea()
{
    IncompleteArea.x = 0;
    IncompleteArea.y = m_linesShown * m_fontHeight + 1;
    IncompleteArea.width = GetClientSize().GetWidth();
    IncompleteArea.height = GetClientSize().GetHeight() - IncompleteArea.y;
}


/*
 * Take an address and show it
 * in the center of codeview
 */
void CodeView::CenterAddress(uint address)
{
    int     i,
            firstlineshown,
            lastlineshown,
            position;
    bool    needscroll = false;

    firstlineshown = GetFirstLine();
    lastlineshown = firstlineshown + m_linesShown - 1;

    m_CodeViewLine->getLineOfAddress(address, i);

    if (i >= 0)
    {
        position = m_linesShown / 2;
        if ((i >= firstlineshown) && (i <= lastlineshown))
        {
            CursorLastPosition = CursorPosition;
            CursorPosition = i;
        }
        else
        {
            CursorLastPosition = i;
            CursorPosition = i;
            needscroll = true;
        }

        RefreshRect(*LastCursorRect);

        if (needscroll)
            Scroll(-1, (i - position));
        RefreshRect(CalcCursorRfshRect());
    }
}




void CodeView::Plot(void)
{
    Refresh();
}


void CodeView::CalcItemsShown(void)
{
    wxSize size = GetClientSize();
    m_linesShown = size.GetHeight() / m_fontHeight;
}



int CodeView::GetFirstLine()
{
    int ret;
    GetViewStart(NULL, &ret);
    return ret;
}




int CodeView::GetLastLine()
{
    return (GetFirstLine() + m_linesShown - 1);
}


int CodeView::GetLastItem()
{
    return (Process->CodeViewLines->GetCount() - 1);
}


void CodeView::UpdateLastCursorRect()
{
    if (LastCursorRect != 0)
    {
        wxCoord width;
        GetClientSize(&width, 0);

        LastCursorRect->x = 2;
        LastCursorRect->y = CursorPosition * m_fontHeight;
        LastCursorRect->width = width - 4;
        LastCursorRect->height = m_fontHeight;
        CalcScrolledPosition(0, LastCursorRect->y, 0, &LastCursorRect->y);
    }
}



void CodeView::UpdateVirtualSize(void)
{
	if (IsEnabled())
	{
		wxSize sz = GetVirtualSize();
		sz.y = m_CodeViewLine->GetCount() * m_fontHeight;
		SetVirtualSize(sz);
	}
}



void CodeView::ShowCursor(wxDC &dc)
{
    wxRect cursor;
    UpdateLastCursorRect();
    cursor = *LastCursorRect;
    CalcUnscrolledPosition(cursor.x, cursor.y, 0, &cursor.y);
    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(*wxLIGHT_GREY_BRUSH);
    dc.DrawRectangle(cursor);
}



void CodeView::ClearCursor()
{
    if ((CursorLastPosition >= 0) && (CursorLastPosition <= GetLastLine()))
        RefreshRect(*LastCursorRect);
}








ElementType CodeView::GetTypeMultiselection(bool &hcomment)

{
    CodeViewItem *cvi;
    DAsmElement *de;
    int i;
    bool homogeneous = false;
    ElementType lastitem = et_None;


    for (i = SelectedItemIndex; i < SelectedLastItem; i++)
    {
        cvi = m_CodeViewLine->getData(i);
        if (cvi->Dasmitem >= 0)
        {
			if (cvi->Comment != 0)
				hcomment = true;

            de = Process->Disassembled->GetData(cvi->Dasmitem);
            if (de->isInstruction())
            {
                if ((lastitem == et_None) || (lastitem == et_Instruction))
                {
                    lastitem = et_Instruction;
                    homogeneous = true;
                }
                else
                    homogeneous = false;
            }
            else
                if (de->isData())
                {
                    if ((lastitem == et_None) || (lastitem == et_Data))
                    {
                        lastitem = et_Data;
                        homogeneous = true;
                    }
                    else
                        homogeneous = false;
                }
            if ((lastitem != et_None) && (!homogeneous))
                break;
        }
    }
    if ((lastitem != et_None) && homogeneous)
        return lastitem;
    return (et_None);
}




/* Returns the first and the last line of instruction / data
 * Returns program labels
 */
bool CodeView::FilterInstructions(wxArrayInt &range, wxArrayInt *plabels, wxArrayInt *vlabels)
{
    bool	foundindex;
    int		i, last_i;
    CodeViewItem
			*cvi;
    DAsmElement
			*de;


    foundindex = false;
    last_i = 0;
    for (i = SelectedItemIndex; i <= SelectedLastItem; i++)
    {
        cvi = m_CodeViewLine->getData(i);
        if (cvi->Dasmitem >= 0)
        {
            de = Process->Disassembled->GetData(cvi->Dasmitem);
            if (de->isInstruction() ||
                de->isData())
            {
                if (!foundindex)
                {
                    range.Add(i);
                    foundindex = true;
                }
                last_i = i;
            }
        }
        if ((plabels != 0) && (cvi->LabelProgAddr >= 0))
		{
			plabels->Add(cvi->LabelProgAddr);
			LogIt(wxString::Format("Label to delete: %.4x\n", cvi->LabelProgAddr));
		}
		if (vlabels && (cvi->LabelVarAddr >= 0))
        {
            vlabels->Add(cvi->LabelVarAddr);
            LogIt(wxString::Format("VAR Label to SAVE: %.4x\n", cvi->LabelVarAddr));
        }
    }
    if (foundindex)
        range.Add(last_i);

	if (range.GetCount() > 0)
		return true;
	else
		return false;
}



void CodeView::CreatePopupMenuMultiSelection(wxMenu *popup)
{
    bool    hascomments;
    wxMenu  *organizeDataSubMenu = 0;

    switch (GetTypeMultiselection(hascomments))
    {
        case et_Instruction:
                        popup->Append(idPOPUP_MAKEDATA, "Make data\td");
                        break;
        case et_Data:
                        organizeDataSubMenu = new wxMenu();
                        popup->Append(idPOPUP_DISASM, "Disassemble\tc");
                        organizeDataSubMenu->Append(idPOPUP_OD_STRING, "String");
                        organizeDataSubMenu->Append(idPOPUP_OD_MATRIX, "Matrix");
                        organizeDataSubMenu->Append(idPOPUP_OD_NUMBER, "Number");
                        popup->AppendSeparator();
                        popup->Append(idPOPUP_ORGANIZEDATA, "Organize data", organizeDataSubMenu);
                        break;
        default:
                        popup->Append(idPOPUP_MAKEDATA, "Make it data\td");
                        break;
    }
    if (hascomments)
    {
        popup->AppendSeparator();
        popup->Append(idPOPUP_DELCOMMENT,  "Del comments");
    }
}



void CodeView::CreatePopupMenuSingleSelection(wxMenu *popup)
{
    DAsmElement *de;
    wxMenu		*argStyleSubMenu = 0,
				*labelMenu = 0;

    de = m_iteminfo.dasmitem;

    popup->Append(idPOPUP_GOTO_ADDRESS, "Goto address...");

    switch(m_iteminfo.type)
    {
        case 	siInstructionLabel:
                                if ((de != 0) &&
                                    (de->MnemonicObject->isCall() || de->MnemonicObject->isJump()))
                                {
                                    popup->Append(idPOPUP_GOTO, "Goto label");
                                    popup->AppendSeparator();
                                }
        case	siLineLabelProg:
        case	siLineLabelVar:
                                labelMenu = new wxMenu();
                                labelMenu->Append(idPOPUP_EDITLABEL, "Edit");
                                labelMenu->AppendSeparator();
                                labelMenu->Append(idPOPUP_DELLABEL, "Delete");

        case	siInstruction:
                                if ((m_iteminfo.type != siLineLabelProg) &&
                                    (m_iteminfo.type != siLineLabelVar))
                                    popup->Append(idPOPUP_MAKEDATA, "Make data");
                                break;
        case	siData:
                                popup->Append(idPOPUP_DISASM, "Disassemble");


    }

    if (labelMenu != 0)
        popup->Append(idPOPUP_LBL, "Label", labelMenu);

    // Clicked over an argument
    if (m_iteminfo.argSelected > 0)
    {
        argStyleSubMenu = new wxMenu();

        argStyleSubMenu->Append(idPOPUP_ARG_BIN, "Binary");
        argStyleSubMenu->Append(idPOPUP_ARG_DEC, "Decimal");
        argStyleSubMenu->Append(idPOPUP_ARG_HEX, "Hexadecimal");
        popup->Append(idPOPUP_ARG_STYLE, "Style data", argStyleSubMenu);
    }

    popup->AppendSeparator();

    if ((m_iteminfo.type == siComments) || (m_iteminfo.hasComment))
    {
        popup->Append(idPOPUP_EDITCOMMENT, "Edit comment");
        popup->Append(idPOPUP_DELCOMMENT, "Del comment");
    }
    else
        popup->Append(idPOPUP_ADDCOMMENT, "Add comment");

}





void CodeView::FillSelectedItemInfo(const wxPoint &pt)
{
	DAsmElement *de;
	CodeViewItem *cvi;
	bool testcomment = false;
	wxPoint mousept;

	CalcUnscrolledPosition(pt.x, pt.y, &mousept.x, &mousept.y);

	m_iteminfo.type = siUnknown;

	cvi = m_CodeViewLine->getData(CursorPosition);
	if (cvi != 0)
	{
		m_iteminfo.lineitem = cvi;
		// Does the line have comment ?
		if (cvi->Comment != 0)
			m_iteminfo.hasComment = true;
		else
			m_iteminfo.hasComment = false;

		// If line is a program label or a var label
		if (cvi->LabelProgAddr >= 0)
			m_iteminfo.type = siLineLabelProg;

		if (cvi->LabelVarAddr >= 0)
			m_iteminfo.type = siLineLabelVar;

		// If clicked over an instruction argument, discover which was
		if ((cvi->RectArg1 != 0) && (cvi->RectArg1->Contains(mousept)))
			m_iteminfo.argSelected = 1;
		else
			m_iteminfo.argSelected = 0;

		if ((cvi->RectArg2 != 0) && (cvi->RectArg2->Contains(mousept)))
			m_iteminfo.argSelected = 2;

		// check if line has instruction
		if (cvi->Dasmitem >= 0)
		{
			de = Process->Disassembled->GetData(cvi->Dasmitem);
			if (de != 0)
			{
				m_iteminfo.dasmitem = de;
				if (de->HasArgumentLabel())
					m_iteminfo.type = siInstructionLabel;
				else
					m_iteminfo.type = siInstruction;

				if (de->isData())
					m_iteminfo.type = siData;

			}
			else
				m_iteminfo.dasmitem = (DAsmElement *)0;
		}
		else
				m_iteminfo.dasmitem = (DAsmElement *)0;

		// Test if line has only comments
		testcomment = ((cvi->Org + cvi->LabelProgAddr + cvi->LabelVarAddr +
						cvi->Dasmitem) == -4);
		if (testcomment)
			m_iteminfo.type = siComments;
	}
	else
		m_iteminfo.lineitem = (CodeViewItem *)0;
}

