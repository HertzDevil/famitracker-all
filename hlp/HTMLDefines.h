 
// Commands (ID_* and IDM_*) 
#define HID_MANIFEST                            0x10001
#define HID_INVALID_WAVEFILE                    0x10003
#define HID_NEXT_FRAME                          0x1007D
#define HID_PREV_FRAME                          0x1007E
#define HID_CMD_OCTAVE_NEXT                     0x10080
#define HID_CMD_OCTAVE_PREVIOUS                 0x10081
#define HID_CMD_NEXT_INSTRUMENT                 0x10084
#define HID_CMD_PREV_INSTRUMENT                 0x10085
#define HID_CMD_INCREASESTEPSIZE                0x10086
#define HID_CMD_DECREASESTEPSIZE                0x10087
#define HID_TRACKER_TOGGLE_PLAY                 0x10088
#define HID_TRACKER_PLAY_START                  0x1008A
#define HID_FOCUS_FRAME_EDITOR                  0x1008B
#define HID_FOCUS_PATTERN_EDITOR                0x1008C
#define HID_CMD_STEP_UP                         0x1008D
#define HID_CMD_STEP_DOWN                       0x1008E
#define HID_DELETE_ROW                          0x1008F
#define HID_BLOCK_START                         0x10090
#define HID_BLOCK_END                           0x10091
#define HID_CLONE_SEQUENCE                      0x100AF
#define HID_MODULE_TEST_EXPORT                  0x100B0
#define HID_INCREASEVALUES                      0x100B1
#define HID_DECREASEVALUES                      0x100B2
#define HID_TRACKER_PLAY_CURSOR                 0x1012C
#define HID_TRACKER_PLAY                        0x18003
#define HID_TRACKER_PLAYPATTERN                 0x18007
#define HID_TRACKER_STOP                        0x18008
#define HID_TRACKER_EDIT                        0x18009
#define HID_TRACKER_KILLSOUND                   0x1800A
#define HID_FILE_CREATE_NSF                     0x18027
#define HID_FILE_SOUNDSETTINGS                  0x18029
#define HID_EDIT_DELETE                         0x18034
#define HID_EDIT_UNLOCKCURSOR                   0x18037
#define HID_TRACKER_NTSC                        0x18038
#define HID_TRACKER_PAL                         0x18039
#define HID_SPEED_DEFALUT                       0x1803B
#define HID_SPEED_CUSTOM                        0x1803C
#define HID_HELP_PERFORMANCE                    0x1803D
#define HID_EDIT_PASTEOVERWRITE                 0x1803F
#define HID_TRANSPOSE_INCREASENOTE              0x18041
#define HID_TRANSPOSE_DECREASENOTE              0x18042
#define HID_TRANSPOSE_INCREASEOCTAVE            0x18043
#define HID_TRANSPOSE_DECREASEOCTAVE            0x18044
#define HID_EDIT_SELECTALL                      0x1804F
#define HID_FILE_GENERALSETTINGS                0x18053
#define HID_EDIT_ENABLEMIDI                     0x18055
#define HID_FRAME_INSERT                        0x18056
#define HID_FRAME_REMOVE                        0x18057
#define HID_TRACKER_PLAYROW                     0x1805A
#define HID_FILE_IMPORTMIDI                     0x1805F
#define HID_CREATE_NSF                          0x18063
#define HID_BUTTON32892                         0x1807C
#define HID_SPEED_DEFAULT                       0x1807D
#define HID_MODULE_MODULEPROPERTIES             0x1807F
#define HID_EDIT_PASTEMIX                       0x18083
#define HID_MODULE_MOVEFRAMEDOWN                0x18086
#define HID_MODULE_MOVEFRAMEUP                  0x18087
#define HID_MODULE_SAVEINSTRUMENT               0x1808D
#define HID_MODULE_LOADINSTRUMENT               0x1808F
#define HID_MODULE_ADDINSTRUMENT                0x18093
#define HID_MODULE_REMOVEINSTRUMENT             0x18094
#define HID_TRACKER_SOLOCHANNEL                 0x180AC
#define HID_TRACKER_TOGGLECHANNEL               0x180AD
#define HID_EDIT_GRADIENT                       0x180AE
#define HID_MODULE_EDITINSTRUMENT               0x180AF
#define HID_NEXT_SONG                           0x180B0
#define HID_PREV_SONG                           0x180B1
#define HID_EDIT_INSTRUMENTMASK                 0x180B4
#define HID_TRACKER_SWITCHTOTRACKINSTRUMENT     0x180B5
#define HID_FRAME_INSERT_UNIQUE                 0x180B8
#define HID_FILE_CREATEWAV                      0x180BA
#define HID_VIEW_CONTROLPANEL                   0x180BC
#define HID_EDIT_CLEARPATTERNS                  0x180BE
#define HID_EDIT_INTERPOLATE                    0x180BF
#define HID_POPUP_SOLOCHANNEL                   0x180C2
#define HID_POPUP_TOGGLECHANNEL                 0x180C3
#define HID_MODULE_DUPLICATEFRAME               0x180C4
#define HID_POPUP_DELETE                        0x180D6
#define HID_EDIT_REVERSE                        0x180D8
#define HID_TRACKER_DPCM                        0x180DB
#define HID_INSTRUMENT_ADD                      0x180E3
#define HID_INSTRUMENT_REMOVE                   0x180E4
#define HID_INSTRUMENT_EDIT                     0x180E5
#define HID_INSTRUMENT_CLONE                    0x180E6
#define HID_FRAME_COPY                          0x180E8
#define HID_FRAME_PASTE                         0x180E9
#define HID_EDIT_REPLACEINSTRUMENT              0x180EA
#define HID_CLEANUP_REMOVEUNUSEDPATTERNS        0x180F8
#define HID_HELP_EFFECTTABLE                    0x180F9
#define HID_CLEANUP_REMOVEUNUSEDINSTRUMENTS     0x180FE
#define HID_POPUP_SAMPLEGRAPH2                  0x18102
#define HID_POPUP_SPECTRUMANALYZER              0x18103
#define HID_POPUP_NOTHING                       0x18104
#define HID_POPUP_SAMPLEGRAPH1                  0x18105
#define HID_POPUP_PICKUPROW                     0x18108
#define HID_POPUP_UNMUTEALLCHANNELS             0x1810D
#define HID_MODULE_CHANNELS                     0x1810E
#define HID_INSTRUMENT_                         0x1810F
#define HID_HELP_CHECKFORNEWVERSIONS            0x18110
#define HID_EDIT_EXPANDPATTERN                  0x18111
#define HID_EDIT_SHRINKPATTERN                  0x18112
#define HID_EDIT_EXPANDPATTERN33043             0x18113
#define HID_EDIT_SHRINKPATTERN33044             0x18114
#define HID_Menu33045                           0x18115
#define HID_FILE_IMPORT                         0x18116
#define HID_MODULE_FRAME_INSERT                 0x18117
#define HID_MODULE_FRAME_REMOVE                 0x18118
#define HID_MODULE_HEJ                          0x18119
#define HID_MODULE_REMOVEFRAME                  0x1811A
#define HID_MODULE_COMMENTS                     0x1811B
#define HID_MODULE_INSERTFRAME                  0x1811C
#define HID_INSTRUMENT_NEW                      0x1811D
#define HID_INSTRUMENT_LOAD                     0x1811E
#define HID_INSTRUMENT_SAVE                     0x1811F
#define HID_POPUP_DECAY                         0x18129
#define HID_DECAY_SLOW                          0x1812A
#define HID_DECAY_FAST                          0x1812B
#define HID_INSTRUMENT_CLONEINSTRUMENTSEQUENCES 0x1812C
#define HID_Menu                                0x1812D
#define HID_INSTRUMENT_DEEPCLONE                0x1812E
#define HID_INSTRUMENT_CLONEINSTRUMENTSEQUENCES33071	0x1812F
#define HID_MODULE_DUPLICATEPATTERNS            0x18130
#define HID_FRAME_DUPLICATEPATTERNS             0x18131
#define HID_MODULE_DUPLICATEFRAMEPATTERNS       0x18132
#define HID_EDIT_EXPANDPATTERNS                 0x18133
#define HID_EDIT_SHRINKPATTERNS                 0x18134
#define HID_POPUP_EXPANDPATTERN                 0x18135
#define HID_POPUP_SHRINKPATTERNS                0x18136
#define HID_TRACKER_PLAYFROMSTART               0x18137
#define HID_VIEW_FRAMEEDITOR                    0x18138
#define HID_FRAMEEDITOR_TOP                     0x18139
#define HID_FRAMEEDITOR_LEFT                    0x1813A
#define HID_POPUP_TRANSPOSE                     0x1813B
#define HID_TRANSPOSE_INCREASENOTE33084         0x1813C
#define HID_TRANSPOSE_DECREASENOTE33085         0x1813D
#define HID_TRANSPOSE_INCRESAEOCTAVE            0x1813E
#define HID_TRANSPOSE_DECREASEOCTAVE33087       0x1813F
#define HID_TRANSOSE_INCREASEOCTAVE             0x18140
#define HID_TRACKER_PLAYFROMCURSOR              0x18141
#define HID_TOGGLE_SPEED                        0x18144
#define HID_POPUP_COPYVOLUMEVALUES              0x18149
#define HID_Menu33098                           0x1814A
#define HID_FRAME_CUT                           0x1814B
#define HID_FILE_TESTTEST                       0x1814C
#define HID_FRAME_DELETE                        0x1814D
#define HID_FILE_IMPORTTEXT                     0x18150
#define HID_FILE_EXPORTTEXT                     0x18151
#define HID_CLEANUP_MERGEDUPLICATEDPATTERNS     0x18152
#define HID_POPUP_SAMPLESCOPE2                  0x18153
#define HID_POPUP_SAMPLESCOPE1                  0x18154
#define HID_EDIT_VOLUMEMASK                     0x18157
#define HID_POPUP_HIDECHANNEL                   0x18158
#define HID_EDIT_PASTE33113                     0x18159
#define HID_EDIT_PASTEMIXED                     0x1815A
#define HID_FRAME_PASTEASNEWPA                  0x1815B
#define HID_FRAME_PASTENEWPATTERNS              0x1815C
#define HID_POPUP_DELETE33117                   0x1815D
#define HID_POPUP_TRIM                          0x1815E
#define HID_POPUP_PLAY                          0x1815F
#define HID_INSTRUMENT_SAVETOFILE               0x18160
#define HID_INSTRUMENT_LOADFROMFILE             0x18161
#define HID_HELP_FAQ                            0x18162
#define HID_POPUP_CLONESEQUENCE                 0x18165
#define HID_TRACKER_DISPLAYREGISTERSTATE        0x18166
#define HID_INSTRUMENT_ADD_2A03                 0x19000
#define HID_INSTRUMENT_ADD_FDS                  0x19001
#define HID_INSTRUMENT_ADD_MMC5                 0x19002
#define HID_INSTRUMENT_ADD_N163                 0x19003
#define HID_INSTRUMENT_ADD_S5B                  0x19004
#define HID_INSTRUMENT_ADD_VRC6                 0x19005
#define HID_INSTRUMENT_ADD_VRC7                 0x19006
#define HID_INDICATOR_CHIP                      0x1D000
#define HID_INDICATOR_INSTRUMENT                0x1D001
#define HID_INDICATOR_OCTAVE                    0x1D002
#define HID_INDICATOR_RATE                      0x1D003
#define HID_INDICATOR_TEMPO                     0x1D004
#define HID_INDICATOR_TIME                      0x1D005
#define HID_INDICATOR_POS                       0x1D006
 
// Prompts (IDP_*) 
 
// Resources (IDR_*) 
#define HIDR_MAINFRAME                          0x20080
#define HIDR_FamiTrackerTYPE                    0x20081
#define HIDR_PATTERN_POPUP                      0x200C6
#define HIDR_FRAME_POPUP                        0x200F0
#define HIDR_INSTRUMENT_POPUP                   0x20106
#define HIDR_SAMPLES_POPUP                      0x2010A
#define HIDR_SAMPLE_WND_POPUP                   0x20117
#define HIDR_INSTRUMENT_TOOLBAR                 0x20118
#define HIDR_PATTERN_HEADER_POPUP               0x2011D
#define HIDR_FRAMEWND                           0x20123
#define HIDR_SAMPLE_EDITOR_POPUP                0x20130
#define HIDR_EXPORT_TEST_REPORT                 0x20137
#define HIDR_SEQUENCE_POPUP                     0x2013F
 
// Dialogs (IDD_*) 
#define HIDD_ABOUTBOX                           0x20064
#define HIDD_MAINFRAME                          0x20083
#define HIDD_PERFORMANCE                        0x20091
#define HIDD_SPEED                              0x20092
#define HIDD_PCMIMPORT                          0x20093
#define HIDD_INSTRUMENT_INTERNAL                0x2009E
#define HIDD_INSTRUMENT_DPCM                    0x2009F
#define HIDD_INSTRUMENT                         0x200A0
#define HIDD_CONFIG_APPEARANCE                  0x200B4
#define HIDD_CONFIG_GENERAL                     0x200B7
#define HIDD_PROPERTIES                         0x200B9
#define HIDD_CONFIG_MIDI                        0x200C2
#define HIDD_CONFIG_SOUND                       0x200C9
#define HIDD_CONFIG_SHORTCUTS                   0x200D3
#define HIDD_OCTAVE                             0x200D4
#define HIDD_EXPORT                             0x200DA
#define HIDD_INSTRUMENT_VRC7                    0x200E2
#define HIDD_CREATEWAV                          0x200E8
#define HIDD_WAVE_PROGRESS                      0x200E9
#define HIDD_MAINBAR                            0x200EB
#define HIDD_INSTRUMENT_FDS                     0x200F8
#define HIDD_FRAMEBAR                           0x200FB
#define HIDD_FRAMECONTROLS                      0x200FB
#define HIDD_SAMPLE_EDITOR                      0x20107
#define HIDD_INSTRUMENT_FDS_ENVELOPE            0x20108
#define HIDD_CHANNELS                           0x2011A
#define HIDD_HEADER                             0x2011B
#define HIDD_COMMENTS                           0x20120
#define HIDD_CONFIG_LEVELS                      0x20124
#define HIDD_CONFIG_MIXER                       0x20124
#define HIDD_INSTRUMENT_N163_WAVE1              0x20125
#define HIDD_INSTRUMENT_N163_WAVE               0x20125
#define HIDD_IMPORT                             0x2012D
#define HIDD_EXPORT_TEST                        0x20134
 
// Frame Controls (IDW_*) 
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFX_HH_H__
#define __AFX_HH_H__

#pragma once

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif

// Non-Client HitTest help IDs
#define HID_HT_NOWHERE                          0x40000
#define HID_HT_CAPTION                          0x40002
#define HID_HT_SIZE                             0x40004
#define HID_HT_HSCROLL                          0x40006
#define HID_HT_VSCROLL                          0x40007
#define HID_HT_MINBUTTON                        0x40008
#define HID_HT_MAXBUTTON                        0x40009
#define HID_HT_SIZE                             0x4000A // alias: ID_HT_LEFT
#define HID_HT_SIZE                             0x4000B // alias: ID_HT_RIGHT
#define HID_HT_SIZE                             0x4000C // alias: ID_HT_TOP
#define HID_HT_SIZE                             0x4000D // alias: ID_HT_TOPLEFT
#define HID_HT_SIZE                             0x4000E // alias: ID_HT_TOPRIGHT
#define HID_HT_SIZE                             0x4000F // alias: ID_HT_BOTTOM
#define HID_HT_SIZE                             0x40010 // alias: ID_HT_BOTTOMLEFT
#define HID_HT_SIZE                             0x40011 // alias: ID_HT_BOTTOMRIGHT
#define HID_HT_SIZE                             0x40012 // alias: ID_HT_BORDER
#define HID_HT_OBJECT							0x40013
#define HID_HT_CLOSE							0x40014
#define HID_HT_HELP								0x40015

// WM_SYSCOMMAND help IDs
#define HID_SC_SIZE                             0x1EF00
#define HID_SC_MOVE                             0x1EF01
#define HID_SC_MINIMIZE                         0x1EF02
#define HID_SC_MAXIMIZE                         0x1EF03
#define HID_SC_NEXTWINDOW                       0x1EF04
#define HID_SC_PREVWINDOW                       0x1EF05
#define HID_SC_CLOSE                            0x1EF06
#define HID_SC_RESTORE                          0x1EF12
#define HID_SC_TASKLIST                         0x1EF13

// File MRU and aliases
#define HID_FILE_MRU_FILE1                      0x1E110
#define HID_FILE_MRU_FILE1                      0x1E111 // aliases: MRU_2 - MRU_16
#define HID_FILE_MRU_FILE1                      0x1E112
#define HID_FILE_MRU_FILE1                      0x1E113
#define HID_FILE_MRU_FILE1                      0x1E114
#define HID_FILE_MRU_FILE1                      0x1E115
#define HID_FILE_MRU_FILE1                      0x1E116
#define HID_FILE_MRU_FILE1                      0x1E117
#define HID_FILE_MRU_FILE1                      0x1E118
#define HID_FILE_MRU_FILE1                      0x1E119
#define HID_FILE_MRU_FILE1                      0x1E11A
#define HID_FILE_MRU_FILE1                      0x1E11B
#define HID_FILE_MRU_FILE1                      0x1E11C
#define HID_FILE_MRU_FILE1                      0x1E11D
#define HID_FILE_MRU_FILE1                      0x1E11E
#define HID_FILE_MRU_FILE1                      0x1E11F

// Window menu list
#define HID_WINDOW_ALL                          0x1EF1F

// OLE menu and aliases
#define HID_OLE_VERB_1                          0x1E210
#define HID_OLE_VERB_1                          0x1E211 // aliases: VERB_2 -> VERB_16
#define HID_OLE_VERB_1                          0x1E212
#define HID_OLE_VERB_1                          0x1E213
#define HID_OLE_VERB_1                          0x1E214
#define HID_OLE_VERB_1                          0x1E215
#define HID_OLE_VERB_1                          0x1E216
#define HID_OLE_VERB_1                          0x1E217
#define HID_OLE_VERB_1                          0x1E218
#define HID_OLE_VERB_1                          0x1E219
#define HID_OLE_VERB_1                          0x1E21A
#define HID_OLE_VERB_1                          0x1E21B
#define HID_OLE_VERB_1                          0x1E21C
#define HID_OLE_VERB_1                          0x1E21D
#define HID_OLE_VERB_1                          0x1E21E
#define HID_OLE_VERB_1                          0x1E21F

// Commands (HID_*) 
#define HID_FILE_NEW                            0x1E100
#define HID_FILE_OPEN                           0x1E101
#define HID_FILE_CLOSE                          0x1E102
#define HID_FILE_SAVE                           0x1E103
#define HID_FILE_SAVE_AS                        0x1E104
#define HID_FILE_PAGE_SETUP                     0x1E105
#define HID_FILE_PRINT_SETUP                    0x1E106
#define HID_FILE_PRINT                          0x1E107
#define HID_FILE_PRINT_DIRECT                   0x1E108
#define HID_FILE_PRINT_PREVIEW                  0x1E109
#define HID_FILE_UPDATE                         0x1E10A
#define HID_FILE_SAVE_COPY_AS                   0x1E10B
#define HID_FILE_SEND_MAIL                      0x1E10C
#define HID_EDIT_CLEAR                          0x1E120
#define HID_EDIT_CLEAR_ALL                      0x1E121
#define HID_EDIT_COPY                           0x1E122
#define HID_EDIT_CUT                            0x1E123
#define HID_EDIT_FIND                           0x1E124
#define HID_EDIT_PASTE                          0x1E125
#define HID_EDIT_PASTE_LINK                     0x1E126
#define HID_EDIT_PASTE_SPECIAL                  0x1E127
#define HID_EDIT_REPEAT                         0x1E128
#define HID_EDIT_REPLACE                        0x1E129
#define HID_EDIT_SELECT_ALL                     0x1E12A
#define HID_EDIT_UNDO                           0x1E12B
#define HID_EDIT_REDO                           0x1E12C
#define HID_WINDOW_NEW                          0x1E130
#define HID_WINDOW_ARRANGE                      0x1E131
#define HID_WINDOW_CASCADE                      0x1E132
#define HID_WINDOW_TILE_HORZ                    0x1E133
#define HID_WINDOW_TILE_VERT                    0x1E134
#define HID_WINDOW_SPLIT                        0x1E135
#define HID_APP_ABOUT                           0x1E140
#define HID_APP_EXIT                            0x1E141
#define HID_HELP_INDEX                          0x1E142
#define HID_HELP_FINDER                         0x1E143
#define HID_HELP_USING                          0x1E144
#define HID_CONTEXT_HELP                        0x1E145
#define HID_NEXT_PANE                           0x1E150
#define HID_PREV_PANE                           0x1E151
#define HID_FORMAT_FONT                         0x1E160
#define HID_OLE_INSERT_NEW                      0x1E200
#define HID_OLE_EDIT_LINKS                      0x1E201
#define HID_OLE_EDIT_CONVERT                    0x1E202
#define HID_OLE_EDIT_CHANGE_ICON                0x1E203
#define HID_OLE_EDIT_PROPERTIES                 0x1E204
#define HID_VIEW_TOOLBAR                        0x1E800
#define HID_VIEW_STATUS_BAR                     0x1E801
#define HID_RECORD_FIRST                        0x1E900
#define HID_RECORD_LAST                         0x1E901
#define HID_RECORD_NEXT                         0x1E902
#define HID_RECORD_PREV                         0x1E903
#define HID_WIZBACK                             0x13023
#define HID_WIZNEXT                             0x13024
#define HID_WIZFINISH                           0x13025

// Dialogs (AFX_HIDD_*)
#define AFX_HIDD_FILEOPEN                       0x27004
#define AFX_HIDD_FILESAVE                       0x27005
#define AFX_HIDD_FONT                           0x27006
#define AFX_HIDD_COLOR                          0x27007
#define AFX_HIDD_PRINT                          0x27008
#define AFX_HIDD_PRINTSETUP                     0x27009
#define AFX_HIDD_FIND                           0x2700A
#define AFX_HIDD_REPLACE                        0x2700B
#define AFX_HIDD_NEWTYPEDLG                     0x27801
#define AFX_HIDD_PRINTDLG                       0x27802
#define AFX_HIDD_PREVIEW_TOOLBAR                0x27803
#define AFX_HIDD_PREVIEW_SHORTTOOLBAR           0x2780B
#define AFX_HIDD_INSERTOBJECT                   0x27804
#define AFX_HIDD_CHANGEICON                     0x27805
#define AFX_HIDD_CONVERT                        0x27806
#define AFX_HIDD_PASTESPECIAL                   0x27807
#define AFX_HIDD_EDITLINKS                      0x27808
#define AFX_HIDD_FILEBROWSE                     0x27809
#define AFX_HIDD_BUSY                           0x2780A
#define AFX_HIDD_OBJECTPROPERTIES               0x2780C
#define AFX_HIDD_CHANGESOURCE                   0x2780D

// Prompts/Errors (AFX_HIDP_*)
#define AFX_HIDP_NO_ERROR_AVAILABLE             0x3F020
#define AFX_HIDP_INVALID_FILENAME               0x3F100
#define AFX_HIDP_FAILED_TO_OPEN_DOC             0x3F101
#define AFX_HIDP_FAILED_TO_SAVE_DOC             0x3F102
#define AFX_HIDP_ASK_TO_SAVE                    0x3F103
#define AFX_HIDP_FAILED_TO_CREATE_DOC           0x3F104
#define AFX_HIDP_FILE_TOO_LARGE                 0x3F105
#define AFX_HIDP_FAILED_TO_START_PRINT          0x3F106
#define AFX_HIDP_FAILED_TO_LAUNCH_HELP          0x3F107
#define AFX_HIDP_INTERNAL_FAILURE               0x3F108
#define AFX_HIDP_COMMAND_FAILURE                0x3F109
#define AFX_HIDP_FAILED_MEMORY_ALLOC            0x3F10A
#define AFX_HIDP_UNREG_DONE                     0x3F10B
#define AFX_HIDP_UNREG_FAILURE                  0x3F10C
#define AFX_HIDP_DLL_LOAD_FAILED                0x3F10D
#define AFX_HIDP_DLL_BAD_VERSION                0x3F10E
#define AFX_HIDP_PARSE_INT                      0x3F110
#define AFX_HIDP_PARSE_REAL                     0x3F111
#define AFX_HIDP_PARSE_INT_RANGE                0x3F112
#define AFX_HIDP_PARSE_REAL_RANGE               0x3F113
#define AFX_HIDP_PARSE_STRING_SIZE              0x3F114
#define AFX_HIDP_PARSE_RADIO_BUTTON             0x3F115
#define AFX_HIDP_PARSE_BYTE                     0x3F116
#define AFX_HIDP_PARSE_UINT                     0x3F117
#define AFX_HIDP_PARSE_DATETIME                 0x3F118
#define AFX_HIDP_PARSE_CURRENCY                 0x3F119
#define AFX_HIDP_FAILED_INVALID_FORMAT          0x3F120
#define AFX_HIDP_FAILED_INVALID_PATH            0x3F121
#define AFX_HIDP_FAILED_DISK_FULL               0x3F122
#define AFX_HIDP_FAILED_ACCESS_READ             0x3F123
#define AFX_HIDP_FAILED_ACCESS_WRITE            0x3F124
#define AFX_HIDP_FAILED_IO_ERROR_READ           0x3F125
#define AFX_HIDP_FAILED_IO_ERROR_WRITE          0x3F126
#define AFX_HIDP_STATIC_OBJECT                  0x3F180
#define AFX_HIDP_FAILED_TO_CONNECT              0x3F181
#define AFX_HIDP_SERVER_BUSY                    0x3F182
#define AFX_HIDP_BAD_VERB                       0x3F183
#define AFX_HIDP_FAILED_TO_NOTIFY               0x3F185
#define AFX_HIDP_FAILED_TO_LAUNCH               0x3F186
#define AFX_HIDP_ASK_TO_UPDATE                  0x3F187
#define AFX_HIDP_FAILED_TO_UPDATE               0x3F188
#define AFX_HIDP_FAILED_TO_REGISTER             0x3F189
#define AFX_HIDP_FAILED_TO_AUTO_REGISTER        0x3F18A
#define AFX_HIDP_FAILED_TO_CONVERT              0x3F18B
#define AFX_HIDP_GET_NOT_SUPPORTED              0x3F18C
#define AFX_HIDP_SET_NOT_SUPPORTED              0x3F18D
#define AFX_HIDP_ASK_TO_DISCARD                 0x3F18E
#define AFX_HIDP_FAILED_TO_CREATE               0x3F18F
#define AFX_HIDP_FAILED_MAPI_LOAD               0x3F190
#define AFX_HIDP_INVALID_MAPI_DLL               0x3F191
#define AFX_HIDP_FAILED_MAPI_SEND               0x3F192
#define AFX_HIDP_FILE_NONE                      0x3F1A0
#define AFX_HIDP_FILE_GENERIC                   0x3F1A1
#define AFX_HIDP_FILE_NOT_FOUND                 0x3F1A2
#define AFX_HIDP_FILE_BAD_PATH                  0x3F1A3
#define AFX_HIDP_FILE_TOO_MANY_OPEN             0x3F1A4
#define AFX_HIDP_FILE_ACCESS_DENIED             0x3F1A5
#define AFX_HIDP_FILE_INVALID_FILE              0x3F1A6
#define AFX_HIDP_FILE_REMOVE_CURRENT            0x3F1A7
#define AFX_HIDP_FILE_DIR_FULL                  0x3F1A8
#define AFX_HIDP_FILE_BAD_SEEK                  0x3F1A9
#define AFX_HIDP_FILE_HARD_IO                   0x3F1AA
#define AFX_HIDP_FILE_SHARING                   0x3F1AB
#define AFX_HIDP_FILE_LOCKING                   0x3F1AC
#define AFX_HIDP_FILE_DISKFULL                  0x3F1AD
#define AFX_HIDP_FILE_EOF                       0x3F1AE
#define AFX_HIDP_ARCH_NONE                      0x3F1B0
#define AFX_HIDP_ARCH_GENERIC                   0x3F1B1
#define AFX_HIDP_ARCH_READONLY                  0x3F1B2
#define AFX_HIDP_ARCH_ENDOFFILE                 0x3F1B3
#define AFX_HIDP_ARCH_WRITEONLY                 0x3F1B4
#define AFX_HIDP_ARCH_BADINDEX                  0x3F1B5
#define AFX_HIDP_ARCH_BADCLASS                  0x3F1B6
#define AFX_HIDP_ARCH_BADSCHEMA                 0x3F1B7
#define AFX_HIDP_SQL_CONNECT_FAIL               0x3F281
#define AFX_HIDP_SQL_RECORDSET_FORWARD_ONLY     0x3F282
#define AFX_HIDP_SQL_EMPTY_COLUMN_LIST          0x3F283
#define AFX_HIDP_SQL_FIELD_SCHEMA_MISMATCH      0x3F284
#define AFX_HIDP_SQL_ILLEGAL_MODE               0x3F285
#define AFX_HIDP_SQL_MULTIPLE_ROWS_AFFECTED     0x3F286
#define AFX_HIDP_SQL_NO_CURRENT_RECORD          0x3F287
#define AFX_HIDP_SQL_NO_ROWS_AFFECTED           0x3F288
#define AFX_HIDP_SQL_RECORDSET_READONLY         0x3F289
#define AFX_HIDP_SQL_SQL_NO_TOTAL               0x3F28A
#define AFX_HIDP_SQL_ODBC_LOAD_FAILED           0x3F28B
#define AFX_HIDP_SQL_DYNASET_NOT_SUPPORTED      0x3F28C
#define AFX_HIDP_SQL_SNAPSHOT_NOT_SUPPORTED     0x3F28D
#define AFX_HIDP_SQL_API_CONFORMANCE            0x3F28E
#define AFX_HIDP_SQL_SQL_CONFORMANCE            0x3F28F
#define AFX_HIDP_SQL_NO_DATA_FOUND              0x3F290
#define AFX_HIDP_SQL_ROW_UPDATE_NOT_SUPPORTED   0x3F291
#define AFX_HIDP_SQL_ODBC_V2_REQUIRED           0x3F292
#define AFX_HIDP_SQL_NO_POSITIONED_UPDATES      0x3F293
#define AFX_HIDP_SQL_LOCK_MODE_NOT_SUPPORTED    0x3F294
#define AFX_HIDP_SQL_DATA_TRUNCATED             0x3F295
#define AFX_HIDP_SQL_ROW_FETCH                  0x3F296
#define AFX_HIDP_SQL_INCORRECT_ODBC             0x3F297
#define AFX_HIDP_SQL_UPDATE_DELETE_FAILED       0x3F298
#define AFX_HIDP_SQL_DYNAMIC_CURSOR_NOT_SUPPORTED	0x3F299
#define AFX_HIDP_SQL_FIELD_NOT_FOUND            0x3F29A
#define AFX_HIDP_SQL_BOOKMARKS_NOT_SUPPORTED    0x3F29B
#define AFX_HIDP_SQL_BOOKMARKS_NOT_ENABLED      0x3F29C
#define AFX_HIDP_DAO_ENGINE_INITIALIZATION      0x3F2B0
#define AFX_HIDP_DAO_DFX_BIND                   0x3F2B1
#define AFX_HIDP_DAO_OBJECT_NOT_OPEN            0x3F2B2
#define AFX_HIDP_DAO_ROWTOOSHORT                0x3F2B3
#define AFX_HIDP_DAO_BADBINDINFO                0x3F2B4
#define AFX_HIDP_DAO_COLUMNUNAVAILABLE          0x3F2B5

// Frame Controls (AFX_HIDW_*)
#define AFX_HIDW_TOOLBAR                        0x5E800
#define AFX_HIDW_STATUS_BAR                     0x5E801
#define AFX_HIDW_PREVIEW_BAR                    0x5E802
#define AFX_HIDW_RESIZE_BAR                     0x5E803
#define AFX_HIDW_DOCKBAR_TOP                    0x5E81B
#define AFX_HIDW_DOCKBAR_LEFT                   0x5E81C
#define AFX_HIDW_DOCKBAR_RIGHT                  0x5E81D
#define AFX_HIDW_DOCKBAR_BOTTOM                 0x5E81E
#define AFX_HIDW_DOCKBAR_FLOAT                  0x5E81F

/////////////////////////////////////////////////////////////////////////////

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif

#endif // __AFX_HH_H__
