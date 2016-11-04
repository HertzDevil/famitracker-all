// Source\InstrumentEditorN106.cpp : implementation file
//

#include "stdafx.h"
#include "FamiTracker.h"
#include "InstrumentEditPanel.h"
#include "InstrumentEditorN106.h"


// CInstrumentEditorN106 dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorN106, CInstrumentEditPanel)

CInstrumentEditorN106::CInstrumentEditorN106(CWnd* pParent /*=NULL*/)
	: CInstrumentEditPanel(CInstrumentEditorN106::IDD, pParent)
{

}

CInstrumentEditorN106::~CInstrumentEditorN106()
{
}

void CInstrumentEditorN106::DoDataExchange(CDataExchange* pDX)
{
	CInstrumentEditPanel::DoDataExchange(pDX);
}

void CInstrumentEditorN106::SelectInstrument(int Instrument)
{
}

BEGIN_MESSAGE_MAP(CInstrumentEditorN106, CInstrumentEditPanel)
END_MESSAGE_MAP()


// CInstrumentEditorN106 message handlers
