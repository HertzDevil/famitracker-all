/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2010  Jonathan Liss
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#pragma once

class CInstrumentEditorN106 : public CInstrumentEditPanel
{
	DECLARE_DYNAMIC(CInstrumentEditorN106)

public:
	CInstrumentEditorN106(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentEditorN106();
	virtual int GetIDD() { return IDD; };
	virtual TCHAR *GetTitle() { return _T("Namco N106"); };

	// Public
	void SelectInstrument(int Instrument);

// Dialog Data
	enum { IDD = IDD_INSTRUMENT_N106 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
