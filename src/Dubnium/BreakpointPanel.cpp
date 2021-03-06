// {{{ Copyright notice
/* Copyright (c) 2007-2009, Adam Harvey
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *  - The names of its contributors may not be used to endorse or promote
 *  products derived from this software without specific prior written
 *  permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
// }}}

#include "BreakpointPanel.h"
#include "ConnectionPage.h"
#include "Dubnium.h"
#include "FunctionBreakpointDialog.h"

#include <algorithm>

#include <wx/arrstr.h>
#include <wx/artprov.h>
#include <wx/log.h>
#include <wx/textdlg.h>
#include <wx/tokenzr.h>

// {{{ bool IsNotDisplayed(DBGp::Breakpoint *bp)
bool IsNotDisplayed(DBGp::Breakpoint *bp) {
	DBGp::Breakpoint::Type type = bp->GetType();
	return bp->GetTemporary() || type == DBGp::Breakpoint::WATCH || type == DBGp::Breakpoint::CONDITIONAL;
}
// }}}

// {{{ Event table
BEGIN_EVENT_TABLE(BreakpointPanel, wxPanel)
	EVT_GRID_CMD_CELL_LEFT_CLICK(ID_BREAKPOINTPANEL_GRID, BreakpointPanel::OnClickCell)
	EVT_GRID_CMD_SELECT_CELL(ID_BREAKPOINTPANEL_GRID, BreakpointPanel::OnSelectCell)
	EVT_TOOL(ID_BREAKPOINTPANEL_ADD_CALL, BreakpointPanel::OnAddCall)
	EVT_TOOL(ID_BREAKPOINTPANEL_ADD_EXCEPTION, BreakpointPanel::OnAddException)
	EVT_TOOL(ID_BREAKPOINTPANEL_ADD_RETURN, BreakpointPanel::OnAddReturn)
	EVT_TOOL(ID_BREAKPOINTPANEL_ADD_WATCH, BreakpointPanel::OnAddWatch)
	EVT_TOOL(ID_BREAKPOINTPANEL_REMOVE, BreakpointPanel::OnRemove)
END_EVENT_TABLE()
// }}}

// {{{ BreakpointPanel::BreakpointPanel(ConnectionPage *parent, wxWindowID id)
BreakpointPanel::BreakpointPanel(ConnectionPage *parent, wxWindowID id) : ToolbarPanel(parent, id), breakpoints(parent->GetConnection()->GetBreakpoints()) {
	// Retrieve the breakpoint types that the debugging engine supports.
	wxSortedArrayString types;
	try {
		DBGp::Connection *conn = parent->GetConnection();
		wxString typeString(conn->FeatureGet(wxT("breakpoint_types")));

		// Work around for XDebug bug #486.
		if (typeString == wxT("line call return") && conn->FeatureGet(wxT("language_name")) == wxT("PHP")) {
			/* This is a workaround for XDebug bug #486; current
			 * versions of XDebug always return "line call return",
			 * which isn't actually true. */
			types.Add(wxT("line"));
			types.Add(wxT("call"));
			types.Add(wxT("return"));
			types.Add(wxT("conditional"));
			types.Add(wxT("exception"));
		}
		else {
			/* For other setups, we'll believe what the debugging
			 * engine actually tells us. */
			wxStringTokenizer tkz(typeString);
			
			for (wxString type(tkz.GetNextToken()); tkz.HasMoreTokens(); type = tkz.GetNextToken()) {
				types.Add(type);
			}
		}
	}
	catch (...) {
		// We'll proceed, just without any toolbar buttons bar remove.
		wxLogDebug(wxT("Getting breakpoint_types failed; proceeding without breakpoint support."));
	}

	if (types.Index(wxT("call")) != wxNOT_FOUND) {
		toolbar->AddTool(ID_BREAKPOINTPANEL_ADD_CALL, _("Add Function Call Breakpoint"), wxArtProvider::GetBitmap(wxT("breakpoint-function"), wxART_TOOLBAR, size), _("Add a breakpoint triggered by a function call"));
	}

	if (types.Index(wxT("return")) != wxNOT_FOUND) {
		toolbar->AddTool(ID_BREAKPOINTPANEL_ADD_RETURN, _("Add Function Return Breakpoint"), wxArtProvider::GetBitmap(wxT("breakpoint-return"), wxART_TOOLBAR, size), _("Add a breakpoint triggered by a function return"));
	}

	if (types.Index(wxT("exception")) != wxNOT_FOUND) {
		toolbar->AddTool(ID_BREAKPOINTPANEL_ADD_EXCEPTION, _("Add Exception Breakpoint"), wxArtProvider::GetBitmap(wxT("breakpoint-exception"), wxART_TOOLBAR, size), _("Add a breakpoint triggered by an exception being thrown"));
	}

	if (types.Index(wxT("watch")) != wxNOT_FOUND) {
		toolbar->AddTool(ID_BREAKPOINTPANEL_ADD_WATCH, _("Add Watch Breakpoint"), wxArtProvider::GetBitmap(wxT("watch"), wxART_TOOLBAR, size), _("Add a breakpoint triggered by the value of a watched variable changing"));
	}

	if (types.GetCount()) {
		toolbar->AddSeparator();
	}

	toolbar->AddTool(ID_BREAKPOINTPANEL_REMOVE, _("Remove Breakpoint"), wxArtProvider::GetBitmap(wxART_DELETE, wxART_TOOLBAR, size), _("Remove the selected breakpoint"));
	toolbar->Realize();

	grid = new wxGrid(this, ID_BREAKPOINTPANEL_GRID);
	ResetGrid();
	grid->EnableEditing(false);
	grid->SetSelectionMode(wxGrid::wxGridSelectRows);
	sizer->Add(grid, 1, wxEXPAND | wxALL);
}
// }}}

// {{{ DBGp::Breakpoint *BreakpointPanel::GetFileBreakpoint(const wxString &file, int line)
DBGp::Breakpoint *BreakpointPanel::GetFileBreakpoint(const wxString &file, int line) {
	DBGp::Breakpoint *bp = NULL;
	for (DBGp::Connection::BreakpointList::iterator i = breakpoints.begin(); !bp && i != breakpoints.end(); i++) {
		if ((*i)->GetType() == DBGp::Breakpoint::LINE && (*i)->GetFileName() == file && (*i)->GetLineNo() == line) {
			bp = *i;
		}
	}
	return bp;
}
// }}}
// {{{ DBGp::Connection::BreakpointList BreakpointPanel::GetFileBreakpoints(const wxString &file)
DBGp::Connection::BreakpointList BreakpointPanel::GetFileBreakpoints(const wxString &file) {
	DBGp::Connection::BreakpointList list;
	for (DBGp::Connection::BreakpointList::iterator i = breakpoints.begin(); i != breakpoints.end(); i++) {
		if ((*i)->GetType() == DBGp::Breakpoint::LINE && (*i)->GetFileName() == file) {
			list.push_back(*i);
		}
	}
	return list;
}
// }}}
// {{{ void BreakpointPanel::Update()
void BreakpointPanel::Update() {
	breakpoints = parent->GetConnection()->GetBreakpoints();
	std::remove_if(breakpoints.begin(), breakpoints.end(), IsNotDisplayed);

	grid->Freeze();

	if (grid->GetNumberRows() > 0) {
		grid->DeleteRows(0, grid->GetNumberRows());
	}

	grid->AppendRows(breakpoints.size());
	size_t row = 0;
	for (DBGp::Connection::BreakpointList::iterator i = breakpoints.begin(); i != breakpoints.end(); i++, row++) {
		wxString cond;

		switch ((*i)->GetType()) {
			case DBGp::Breakpoint::CALL:
			case DBGp::Breakpoint::RETURN:
			case DBGp::Breakpoint::EXCEPTION:
				grid->SetCellValue(row, 0, wxGetApp().IsStickyBreakpoint(parent->GetScript(), *i) ? wxT("1") : wxT("0"));
				grid->SetCellAlignment(row, 0, wxALIGN_CENTRE, wxALIGN_CENTRE);
				grid->SetCellRenderer(row, 0, new wxGridCellBoolRenderer);
				break;

			default:
				break;
		}

		grid->SetCellValue(row, 1, DBGp::Breakpoint::TypeToString((*i)->GetType()));

		switch ((*i)->GetType()) {
			case DBGp::Breakpoint::CALL:
			case DBGp::Breakpoint::RETURN:
				cond << (*i)->GetFunction();
				break;

			case DBGp::Breakpoint::EXCEPTION:
				cond << (*i)->GetException();
				break;

			case DBGp::Breakpoint::LINE:
				cond << (*i)->GetFileName() << wxT(":") << (*i)->GetLineNo();
				break;

			default:
				cond = wxT("???");
		}
		grid->SetCellValue(row, 2, cond);
		grid->SetRowLabelValue(row, (*i)->GetID());
	}
	grid->AutoSizeColumns();

	grid->Thaw();
}
// }}}

// {{{ void BreakpointPanel::OnAddCall(wxCommandEvent &event)
void BreakpointPanel::OnAddCall(wxCommandEvent &event) {
	FunctionBreakpointDialog fbd(this, _("Add Function Call Breakpoint"));

	if (fbd.ShowModal()) {
		wxString function(fbd.GetFunction());

		if (fbd.IsMethod()) {
			function.Prepend(fbd.GetClass() + wxT("::"));
		}

		DBGp::Breakpoint *bp = parent->GetConnection()->CreateBreakpoint();
		bp->SetCallType(function);
		bp->Set();
	}

	Update();
}
// }}}
// {{{ void BreakpointPanel::OnAddException(wxCommandEvent &event)
void BreakpointPanel::OnAddException(wxCommandEvent &event) {
	wxString exception(wxGetTextFromUser(_("Please enter the exception class to break on:"), _("Add Exception Breakpoint"), wxEmptyString, this));
	if (exception != wxEmptyString) {
		DBGp::Breakpoint *bp = parent->GetConnection()->CreateBreakpoint();
		bp->SetExceptionType(exception);
		bp->Set();
	}
	Update();
}
// }}}
// {{{ void BreakpointPanel::OnAddReturn(wxCommandEvent &event)
void BreakpointPanel::OnAddReturn(wxCommandEvent &event) {
	FunctionBreakpointDialog fbd(this, _("Add Function Return Breakpoint"));

	if (fbd.ShowModal()) {
		wxString function(fbd.GetFunction());

		if (fbd.IsMethod()) {
			function.Prepend(fbd.GetClass() + wxT("::"));
		}

		DBGp::Breakpoint *bp = parent->GetConnection()->CreateBreakpoint();
		bp->SetReturnType(function);
		bp->Set();
	}

	Update();
}
// }}}
// {{{ void BreakpointPanel::OnAddWatch(wxCommandEvent &event)
void BreakpointPanel::OnAddWatch(wxCommandEvent &event) {
	wxString watch(wxGetTextFromUser(_("Please enter the variable to watch:"), _("Add Watch Breakpoint"), wxEmptyString, this));
	if (watch != wxEmptyString) {
		DBGp::Breakpoint *bp = parent->GetConnection()->CreateBreakpoint();
		bp->SetWatchType(watch);
		bp->Set();
	}
	Update();
}
// }}}
// {{{ void BreakpointPanel::OnClickCell(wxGridEvent &event)
void BreakpointPanel::OnClickCell(wxGridEvent &event) {
	if (event.GetCol() == 0) {
		wxString id(grid->GetRowLabelValue(event.GetRow()));
		DBGp::Connection *conn = parent->GetConnection();
		DBGp::Breakpoint *bp = conn->GetBreakpoint(id);

		if (bp) {
			switch (bp->GetType()) {
				case DBGp::Breakpoint::CALL:
				case DBGp::Breakpoint::RETURN:
				case DBGp::Breakpoint::EXCEPTION:
					{
						bool isSticky = (grid->GetCellValue(event.GetRow(), event.GetCol()) != wxT("0"));
						wxString cellValue(wxT("0"));

						if (isSticky) {
							wxGetApp().RemoveStickyBreakpoint(parent->GetScript(), bp);
						}
						else {
							wxGetApp().AddStickyBreakpoint(parent->GetScript(), bp);
							cellValue = wxT("1");
						}
						
						grid->SetCellValue(event.GetRow(), 0, cellValue);

						break;
					}

				default:
					wxLogDebug(wxT("Breakpoint type does not support stickiness."));
			}
		}
	}
}
// }}}
// {{{ void BreakpointPanel::OnRemove(wxCommandEvent &event)
void BreakpointPanel::OnRemove(wxCommandEvent &event) {
	DBGp::Connection *conn = parent->GetConnection();
	wxArrayInt rows(grid->GetSelectedRows());

	for (size_t i = 0; i < rows.GetCount(); i++) {
		wxLogDebug(wxT("Attempting to remove breakpoint on row %d"), i);
		wxString id(grid->GetRowLabelValue(rows.Item(i)));
		DBGp::Breakpoint *bp = conn->GetBreakpoint(id);
		
		if (bp) {
			try {
				if (bp->GetType() == DBGp::Breakpoint::LINE) {
					parent->BreakpointRemove(bp->GetFileName(), bp->GetLineNo());
				}

				conn->RemoveBreakpoint(bp);
			}
			catch (DBGp::Error e) {
				wxLogDebug(wxT("Attempted to remove bad breakpoint %s."), id.c_str());
			}
		}
	}
	Update();
}
// }}}
// {{{ void BreakpointPanel::OnSelectCell(wxGridEvent &event)
void BreakpointPanel::OnSelectCell(wxGridEvent &event) {
	grid->SelectRow(event.GetRow(), event.ControlDown());
}
// }}}
// {{{ void BreakpointPanel::ResetGrid()
void BreakpointPanel::ResetGrid() {
	grid->Freeze();

	grid->CreateGrid(1, 3);

	grid->SetColLabelValue(0, _("Sticky"));
	grid->SetColLabelValue(1, _("Type"));
	grid->SetColLabelValue(2, _("Condition"));

#ifndef DUBNIUM_DEBUG
	grid->SetRowLabelSize(0);
#endif

	grid->Thaw();
}
// }}}

// vim:set fdm=marker ts=8 sw=8 noet cin:
