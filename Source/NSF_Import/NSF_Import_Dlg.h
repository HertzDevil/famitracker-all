/*
** NSF Importer
** Copyright (C) 2011 Brad Smith (rainwarrior@gmail.com)
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

#ifndef NSF_IMPORT_DLG_H
#define NSF_IMPORT_DLG_H

class CNSFFile; // forward declaration

struct NSF_Import_Settings
{
    int iTrack;
    unsigned int iSeconds;
    bool bDebugWAV;
};

extern bool Do_NSF_Import_Dlg(LPCSTR FileName, CNSFFile& NSFFile, NSF_Import_Settings& Settings);

#endif
