#pragma once

#include "stdafx.h"
#include "FamiTrackerDoc.h"
#include "FamiTracker.h"

class CCustomExporter
{
public:
	CCustomExporter( void );
	CCustomExporter( const CCustomExporter& other );
	CCustomExporter& operator=( const CCustomExporter& other );
	~CCustomExporter( void );
	bool load( CString FileName );
	CString const& getName( void ) const;
	bool Export( CFamiTrackerDocInterface const* doc, LPCSTR fileName ) const;

private:
	CString m_name;
	CString m_dllFilePath;
	HINSTANCE m_dllHandle;
	int *m_referenceCount;
	const char* (__cdecl *m_GetName)( void );
	bool (__cdecl *m_Export)( CFamiTrackerDocInterface const* doc, LPCSTR fileName );
	void incReferenceCount( void );
	void decReferenceCount( void );
};
