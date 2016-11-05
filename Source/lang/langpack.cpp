#include "langpack.hpp"
#include <cstddef>
#include <cstdlib>
#include <algorithm>
#include <codecvt>
#include <locale>
#include <sstream>
#include <fstream>

#ifdef YFW_WINDOWS_LOCALE_FAMITRACKER
#include <afxwin.h>
#else
#include <windows.h>
#endif

// there is a bug in vs2015 where linker says no for a codecvt with char16_t and char32_t

namespace YFW{namespace Windows{namespace Locale
{

static std::u16string AsciiToUTF16(const char *str)
{
	return (const char16_t*)std::wstring_convert<std::codecvt_utf8_utf16<wchar_t, 0x10FFFF, std::little_endian>, wchar_t>().from_bytes(str).c_str();
}
static std::string UTF16ToAscii(const char16_t *str)
{
	return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t, 0x10FFFF, std::little_endian>, wchar_t>().to_bytes((const wchar_t*)str);
}

static void ThrowMalformed(const char *info)
{
	throw std::runtime_error(std::string("[LanguagePack]: file malformed: ") + info);
}

static char32_t ExtractCharacter(const char16_t *&s)
{
	//I hope this is correct :)
	char32_t c;
	uint32_t s1 = *s++;
	if(s1 >= 0xD800 && s1 <= 0xDBFF) //divided in low and high surrogates
	{
		uint32_t s2 = *s++; //get low surrogate
		if(s2 >= 0xDC00 && s2 <= 0xDFFF) //low surrogate can only be in this range...
			c = (((s1 - 0xD800) << 10) | (s2 - 0xDC00)) + 0x10000;
		else //else these two surrogates are treated as two separate characters
		{
			s--;
			c = s1;
		}
	}
	else
		c = s1;
	if(c == 0)
		s--;
	return c;
}

static char32_t RequestCharacter(std::basic_istream<char32_t> &s, bool throwit=true)
{
	char32_t c = 0;
	if(!s.get(c) && throwit)
		ThrowMalformed("EOF while reading identifier");
	return c;
}
static void AddCharacter(std::u16string &s, char32_t c)
{
	if(c > 0xFFFF) //character must be divided in two surrogates
	{
		uint16_t s = (c - 0x10000) & 0x3F;
		s += (s >> 10) + 0xD800; //the high surrogate..
		s += (s & 0x3FF) + 0xDC00; //..and the low one
	}
	else
		s += c & 0xFFFF;
}
static bool isWS(char32_t c) { return c == U' ' || c == U'\n' || c == U'\f' || c == U'\r' || c == U'\t'; }
static void SkipWS(std::basic_istream<char32_t> &s)
{
	s >> std::ws;
}

///////////////////////////////////////////////////////////////////////////////////
// language pack
///////////////////////////////////////////////////////////////////////////

void LanguagePack::LoadPack(std::basic_istream<char32_t> &s)
{
	Identifiers.clear();
	for(;;)
	{
		SkipWS(s);
		bool i = s.bad();
		bool w = s.fail();
		std::u16string Ident, Text;
		char32_t c;
		bool first = true;
		for(;;)
		{
			c = RequestCharacter(s, !first);
			if(c == 0)
				return;
			if(isWS(c) || c == U'{')
				break;
			AddCharacter(Ident, c);
			first = false;
		}
		if(c != U'{')
		{
			SkipWS(s);
			c = RequestCharacter(s);
			if(c != U'{')
				ThrowMalformed("'{' expected");
		}
		for(;;)
		{
			c = RequestCharacter(s);
			if(c == U'}')
				break;
			AddCharacter(Text, c);
		}
		Identifiers[Ident] = Text;
	}
}

static std::unique_ptr<std::basic_streambuf<char32_t>> getStreamUTF32Buffer(std::istream &s)
{
	std::unique_ptr<std::basic_streambuf<uint32_t>> bufptr;

	uint8_t bom[4];
	s.read((char*)&bom, 4);

	if(bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) //utf8
	{
		s.seekg(-1, std::ios::cur);
		bufptr.reset(new std::wbuffer_convert<std::codecvt_utf8<uint32_t>, uint32_t>(s.rdbuf()));
	}
	else if(bom[0] == 0xFF && bom[1] == 0xFE) //utf16le
	{
		s.seekg(-2, std::ios::cur);
		bufptr.reset(new std::wbuffer_convert<std::codecvt_utf16<uint32_t, 0x10FFFF, std::little_endian>, uint32_t>(s.rdbuf()));
	}
	else if(bom[0] == 0xFE && bom[1] == 0xFF) //utf16be
	{
		s.seekg(-2, std::ios::cur);
		bufptr.reset(new std::wbuffer_convert<std::codecvt_utf16<uint32_t>, uint32_t>(s.rdbuf()));
	}
	else //Ascii
	{
		s.seekg(-4, std::ios::cur);
		bufptr.reset(new std::wbuffer_convert<std::codecvt_utf8<uint32_t>, uint32_t>(s.rdbuf())); //just like utf8
	}
	//HACK!!!!!!!!! see note above
	std::unique_ptr<std::basic_streambuf<char32_t>> hack((std::basic_streambuf<char32_t>*)bufptr.release());
	return hack; 
}
bool LanguagePack::LoadFilePack(const char16_t *name)
{
	std::fstream File((const wchar_t*)name, std::ios::in | std::ios::binary);
	if(!File)
		return false;
	std::unique_ptr<std::basic_streambuf<char32_t>> buf = getStreamUTF32Buffer(File);
	LoadPack(std::basic_istream<char32_t>(buf.get()));
	return true;
}
bool LanguagePack::LoadResourcePack(const char16_t *name, const char16_t *type)
{
	HRSRC hRC = FindResourceW(0, (const wchar_t*)name, (wchar_t*)type);
	if(!hRC)
		return false;
	HGLOBAL hGlob = LoadResource(0, hRC);
	const char *ptr = (const char*)LockResource(hGlob);
	DWORD siz = SizeofResource(0, hRC);

	std::stringstream Stream;
	Stream.write(ptr, siz);
	std::unique_ptr<std::basic_streambuf<char32_t>> buf = getStreamUTF32Buffer(Stream);
	LoadPack(std::basic_istream<char32_t>(buf.get()));

	UnlockResource(hGlob);
	FreeResource(hGlob);
	return true;
}
const char16_t *LanguagePack::getString(const char16_t *id) const
{
	auto i = Identifiers.find(id);
	return i == Identifiers.end() ? (const char16_t*)"" : i->second.c_str();
}

std::u16string LanguagePack::ReplaceIndentifiers(const char16_t *s) const
{
	std::u16string n;
	while(*s)
	{
		if(s[0] == u'$' && s[1] == u'!' && s[2])
		{
			s += 2;
			std::u16string id;
			while(*s && *s != u'$')
				AddCharacter(id, ExtractCharacter(s));
			if(*s == u'$')
				s++;
			auto i = Identifiers.find(id);
			n += (i == Identifiers.end()) ? u"[error]" : i->second;
			if(i == Identifiers.end())
				i = i;
		}
		else
			AddCharacter(n, ExtractCharacter(s));
	}
	return n;
}

///////////////////////////////////////////////////////////////////////////////////
// language pack container
///////////////////////////////////////////////////////////////////////////

void LanguagePackSystem::setLocation(const char16_t *path, const char16_t *prefix, const char16_t *suffix)
{
	Path = path;
	Prefix = prefix;
	Suffix = suffix;
	std::u16string findpat = Path + u"\\" + prefix + u"*" + suffix;

	WIN32_FIND_DATAW  ffd;
	HANDLE hFind = FindFirstFileW((const wchar_t*)findpat.c_str(), &ffd);
	if(hFind == INVALID_HANDLE_VALUE)
		return;
	do
	{
		if((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			std::u16string fname = (const char16_t*)ffd.cFileName;
			std::u16string name = fname.substr(Prefix.size(), fname.size() - Prefix.size() - Suffix.size());
			AvailablePacks.push_back(name);
		}
	} while(FindNextFileW(hFind, &ffd) != 0);
}
LanguagePack *LanguagePackSystem::getPack(const char16_t *name)
{
	for(auto i = Packs.begin(); i != Packs.end(); ++i)
	{
		if((*i)->getName() == name)
			return i->get();
	}
	return nullptr;
}
LanguagePack *LanguagePackSystem::RequestPack(const char16_t *name)
{
	LanguagePack *lp = getPack(name);
	if(!lp)
	{
		std::u16string fname = Path + (char16_t)u'\\' + Prefix + name + Suffix;
		std::unique_ptr<LanguagePack> p(new LanguagePack);
		if(!p->LoadFilePack(fname.c_str()))
			return nullptr;
		Packs.emplace_back(p.release());
		lp = Packs.back().get();
	}
	return lp;
}

void LanguagePackSystem::AddPack(const LanguagePack &p)
{
	LanguagePack *e = getPack(p.getName());
	if(e)
		*e = p;
	else
		Packs.emplace_back(new LanguagePack(p));
}
LanguagePack &LanguagePackSystem::AddPack()
{
	Packs.emplace_back(new LanguagePack);
	return *Packs.back();
}

//////////////////////////////////////////////////////////////////////////////////////////////
// global subclass
//////////////////////////////////////////////////////////////////////////////////////////


class GlobalSubclass
{
private:
	WNDCLASSEXW OldClass, NewClass;
	HWND hDummy;
	bool Bypass;
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	virtual LRESULT ProcessWindowMessage(HWND &hWnd, UINT &msg, WPARAM &wParam, LPARAM &lParam) = 0;

public:
	GlobalSubclass(const wchar_t *classname) : Bypass(false)
	{
		WNDCLASSEXW old;
		if(GetClassInfoExW(0, classname, &old) == FALSE)
			throw std::runtime_error("[GlobalSubclass] class not found");
		UnregisterClassW(classname, GetModuleHandleW(0));
		old.cbSize = sizeof(old);
		OldClass = NewClass = old;
		NewClass.lpfnWndProc = WndProc;
		NewClass.cbClsExtra = sizeof(this) + 100;
		RegisterClassExW(&NewClass);
		hDummy = CreateWindowExW(0, classname, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		SetClassLongPtrW(hDummy, 0, (LONG_PTR)this);
	}
	virtual ~GlobalSubclass()
	{
		SetClassLongPtrW(hDummy, 0, 0);
		DestroyWindow(hDummy);
		UnregisterClassW(NewClass.lpszClassName, GetModuleHandleW(0));
		RegisterClassExW(&OldClass);
	}
	void setBypassed(bool b)
	{
		Bypass = b;
	}
	LRESULT CallOldProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return OldClass.lpfnWndProc(hWnd, msg, wParam, lParam);
	}
};

LRESULT GlobalSubclass::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	GlobalSubclass *me = (GlobalSubclass*)GetClassLongPtrW(hWnd, 0);
	if(me)
		return me->Bypass ? me->OldClass.lpfnWndProc(hWnd, msg, wParam, lParam) : me->ProcessWindowMessage(hWnd, msg, wParam, lParam);
	else
		return DefWindowProcW(hWnd, msg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// common subclass
////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
class CommonSubclass : protected GlobalSubclass
{
protected:
	std::map<HWND, T> Controls;

	LRESULT ProcessWindowMessage(HWND &hWnd, UINT &msg, WPARAM &wParam, LPARAM &lParam)
	{
		if(msg == WM_NCCREATE)
			Controls[hWnd] = T();
		else if(msg == WM_DESTROY)
		{
			std::map<HWND, T>::iterator i = Controls.find(hWnd);
			if(i != Controls.end())
				Controls.erase(i);
		}
		return 0;
	}

public:
	CommonSubclass(const wchar_t *classname) : GlobalSubclass(classname) {}
	virtual ~CommonSubclass() {}
	virtual void ChangeLanguage(const LanguagePack *p) = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// text control subclass
////////////////////////////////////////////////////////////////////////////////////////////

struct TextControlInfo
{
	std::u16string OldTitle;
};
class TextControlSubclass : protected CommonSubclass<TextControlInfo>
{
protected:
	const LanguagePack *ActPack;

	LRESULT ProcessWindowMessage(HWND &hWnd, UINT &msg, WPARAM &wParam, LPARAM &lParam)
	{
		CommonSubclass::ProcessWindowMessage(hWnd, msg, wParam, lParam);
		if(msg == WM_NCCREATE)
		{
			std::map<HWND, TextControlInfo>::iterator i = Controls.find(hWnd);
			if(i != Controls.end())
			{
				CREATESTRUCTW c = *(CREATESTRUCTW*)lParam;
				i->second.OldTitle = c.lpszName ? (const char16_t*)c.lpszName : u"";
				std::u16string newtitle = ActPack ? ActPack->ReplaceIndentifiers(i->second.OldTitle.c_str()) : i->second.OldTitle;
				c.lpszName = (const wchar_t*)newtitle.c_str();
				return CallOldProcedure(hWnd, msg, wParam, (LPARAM)&c);
			}
		}
		if(msg == WM_SETTEXT)
		{
			std::map<HWND, TextControlInfo>::iterator i = Controls.find(hWnd);
			if(i != Controls.end())
			{
				i->second.OldTitle = lParam ? (const char16_t*)lParam : u"";
				std::u16string newtitle = ActPack ? ActPack->ReplaceIndentifiers(i->second.OldTitle.c_str()) : i->second.OldTitle;
				return CallOldProcedure(hWnd, msg, wParam, (LPARAM)newtitle.c_str());
			}
		}
		return CallOldProcedure(hWnd, msg, wParam, lParam);
	}

public:
	TextControlSubclass(const wchar_t *classname) : CommonSubclass(classname), ActPack(0) {}
	void ChangeLanguage(const LanguagePack *p)
	{
		for(auto i = Controls.begin(); i != Controls.end(); ++i)
		{
			std::u16string s = p ? p->ReplaceIndentifiers(i->second.OldTitle.c_str()) : i->second.OldTitle.c_str();
			CallOldProcedure(i->first, WM_SETTEXT, 0, (LPARAM)s.c_str());
		}
		ActPack = p;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// static, edit, button subclass
////////////////////////////////////////////////////////////////////////////////////////////

class StaticSubclass : public TextControlSubclass
{
public:
	StaticSubclass() : TextControlSubclass(L"static") {}
};
class EditSubclass : public TextControlSubclass
{
private:
	LRESULT ProcessWindowMessage(HWND &hWnd, UINT &msg, WPARAM &wParam, LPARAM &lParam)
	{
		if(GetWindowLong(hWnd, GWL_STYLE) & ES_READONLY)
			return TextControlSubclass::ProcessWindowMessage(hWnd, msg, wParam, lParam);
		else
			return CallOldProcedure(hWnd, msg, wParam, lParam);
	}

public:
	EditSubclass() : TextControlSubclass(L"edit") {}

};
class ButtonSubclass : public TextControlSubclass
{
public:
	ButtonSubclass() : TextControlSubclass(L"button") {}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// listbox subclass
////////////////////////////////////////////////////////////////////////////////////////////

struct ListBoxInfo
{
	std::vector<std::u16string> OldElements;
};
class ListBoxSubclass : private CommonSubclass<ListBoxInfo>
{
private:
	const LanguagePack *ActPack;

	LRESULT ProcessWindowMessage(HWND &hWnd, UINT &msg, WPARAM &wParam, LPARAM &lParam)
	{
		CommonSubclass::ProcessWindowMessage(hWnd, msg, wParam, lParam);
		auto i = Controls.find(hWnd);
		if(i != Controls.end())
		{
			if(msg == LB_ADDSTRING)
			{
				const char16_t *oldtext = (const char16_t*)lParam;
				std::u16string newtext = ActPack ? ActPack->ReplaceIndentifiers(oldtext) : oldtext;
				LRESULT r = CallOldProcedure(hWnd, LB_ADDSTRING, wParam, (LPARAM)newtext.c_str());
				if((int)r >= 0)
					i->second.OldElements.insert(i->second.OldElements.begin() + (int)r, oldtext);
				return r;
			}
			else if(msg == LB_INSERTSTRING)
			{
				int pos = (int)wParam;
				const char16_t *oldtext = (const char16_t*)lParam;
				std::u16string newtext = ActPack ? ActPack->ReplaceIndentifiers(oldtext) : oldtext;
				LRESULT r = CallOldProcedure(hWnd, LB_INSERTSTRING, wParam, (LPARAM)newtext.c_str());
				if((int)r >= 0)
					i->second.OldElements.insert(i->second.OldElements.begin() + (int)r, oldtext);
				return r;
			}
			else if(msg == LB_DELETESTRING)
			{
				int pos = (int)wParam;
				LRESULT r = CallOldProcedure(hWnd, LB_DELETESTRING, wParam, 0);
				if((int)r >= 0)
					i->second.OldElements.erase(i->second.OldElements.begin() + (int)r);
				return r;
			}
			else if(msg == LB_RESETCONTENT)
			{
				i->second.OldElements.clear();
			}
		}
		return CallOldProcedure(hWnd, msg, wParam, lParam);
	}

public:
	ListBoxSubclass() : CommonSubclass(L"listbox"), ActPack(0) {}

	void ChangeLanguage(const LanguagePack *p)
	{
		ActPack = p;
		for(auto i = Controls.begin(); i != Controls.end(); ++i)
		{
			CallOldProcedure(i->first, WM_SETREDRAW, (WPARAM)false, 0);
			CallOldProcedure(i->first, LB_RESETCONTENT, 0, 0);

			auto v = i->second.OldElements;
			for(auto j = v.begin(); j != v.end(); ++j)
				CallOldProcedure(i->first, LB_ADDSTRING, 0, (LPARAM)(p ? p->ReplaceIndentifiers(j->c_str()).c_str() : j->c_str()));

			CallOldProcedure(i->first, WM_SETREDRAW, (WPARAM)true, 0);
		}
	}

};

///////////////////////////////////////////////////////////////////////////////////////////////////
// listview subclass
////////////////////////////////////////////////////////////////////////////////////////////

struct ListViewInfo
{
	std::vector<std::u16string> OldElements;
};
class ListViewSubclass : private CommonSubclass<ListViewInfo>
{
private:
	const LanguagePack *ActPack;

	LRESULT InsertItemMessage(ListViewInfo &i, HWND hWnd, WPARAM wParam, LPARAM lParam, bool tounicode)
	{
		LVITEMW li = *(LVITEMW*)lParam;

		std::u16string old = tounicode ? AsciiToUTF16((LPSTR)li.pszText) : (const char16_t*)li.pszText;
		std::u16string txt = ActPack ? ActPack->ReplaceIndentifiers(old.c_str()).c_str() : old;
		li.pszText = (wchar_t*)txt.c_str();

		int r = CallOldProcedure(hWnd, LVM_INSERTITEMW, 0, (LPARAM)&li);
		if((int)r >= 0)
			i.OldElements.insert(i.OldElements.begin() + (int)r, old);
		return r;
	}
	LRESULT setItemMessage(ListViewInfo &i, HWND hWnd, WPARAM wParam, LPARAM lParam, int msg)
	{
		LVITEMW li = *(LVITEMW*)lParam;
		if(li.pszText)
		{
			bool u = true;
			if(msg == LVM_SETITEMA)
				msg = LVM_SETITEMW;
			else if(msg == LVM_SETITEMTEXTA)
				msg = LVM_SETITEMTEXTW;
			else
				u = false;

			int p = (int)wParam;
			if(p >= i.OldElements.size())
				return (LPARAM)FALSE;

			std::u16string old = u ? AsciiToUTF16((LPSTR)li.pszText) : (char16_t*)li.pszText;
			std::u16string txt = ActPack ? ActPack->ReplaceIndentifiers(old.c_str()).c_str() : old;
			li.pszText = (wchar_t*)txt.c_str();

			BOOL b = CallOldProcedure(hWnd, msg, wParam, (LPARAM)&li);
			if(b)
				i.OldElements[p] = old;
			return b;
		}
		else
			return CallOldProcedure(hWnd, msg, wParam, lParam);
	}

	LRESULT ProcessWindowMessage(HWND &hWnd, UINT &msg, WPARAM &wParam, LPARAM &lParam)
	{
		CommonSubclass::ProcessWindowMessage(hWnd, msg, wParam, lParam);
		auto i = Controls.find(hWnd);
		if(i != Controls.end())
		{
			if(msg == LVM_INSERTITEMA)
			{
				return InsertItemMessage(i->second, hWnd, wParam, lParam, true);
			}
			else if(msg == LVM_INSERTITEMW)
			{
				return InsertItemMessage(i->second, hWnd, wParam, lParam, false);
			}
			if(msg == LVM_SETITEMA || msg == LVM_SETITEMTEXTA)
			{
				return setItemMessage(i->second, hWnd, wParam, lParam, msg);
			}
			else if(msg == LVM_SETITEMW || msg == LVM_SETITEMTEXTW)
			{
				return setItemMessage(i->second, hWnd, wParam, lParam, msg);
			}
			else if(msg == LVM_DELETEALLITEMS)
			{
				if((BOOL)CallOldProcedure(hWnd, LVM_DELETEALLITEMS, 0, 0) != FALSE)
					i->second.OldElements.clear();
			}
			else if(msg == LVM_DELETEITEM)
			{
				int p = (int)wParam;
				if(p >= i->second.OldElements.size())
					return (LPARAM)FALSE;
				CallOldProcedure(hWnd, LVM_DELETEITEM, wParam, 0);
				i->second.OldElements.erase(i->second.OldElements.begin() + p);
			}
		}
		return CallOldProcedure(hWnd, msg, wParam, lParam);
	}

public:
	ListViewSubclass() : CommonSubclass(WC_LISTVIEWW), ActPack(0) {}

	void ChangeLanguage(const LanguagePack *p)
	{
		ActPack = p;
		LVITEMW lv;
		lv.mask = LVIF_TEXT;
		std::u16string txt;
		for(auto i = Controls.begin(); i != Controls.end(); ++i)
		{
			auto v = i->second.OldElements;
			lv.iItem = 0;
			for(auto j = v.begin(); j != v.end(); ++j)
			{
				txt = p->ReplaceIndentifiers(j->c_str());
				lv.pszText = (wchar_t*)txt.c_str();
				CallOldProcedure(i->first, LVM_SETITEMW, 0, (LPARAM)&lv);
				lv.iItem++;
			}
		}
	}

};


///////////////////////////////////////////////////////////////////////////////////////////////////
// combobox subclass
////////////////////////////////////////////////////////////////////////////////////////////

struct ComboBoxInfo
{
	std::vector<std::u16string> OldElements;
};
class ComboBoxSubclass : private CommonSubclass<ComboBoxInfo>
{
private:
	const LanguagePack *ActPack;

	LRESULT ProcessWindowMessage(HWND &hWnd, UINT &msg, WPARAM &wParam, LPARAM &lParam)
	{
		CommonSubclass::ProcessWindowMessage(hWnd, msg, wParam, lParam);
		auto i = Controls.find(hWnd);
		if(i != Controls.end())
		{
			if(msg == CB_ADDSTRING)
			{
				const char16_t *oldtext = (const char16_t*)lParam;
				std::u16string newtext = ActPack ? ActPack->ReplaceIndentifiers(oldtext) : oldtext;
				LRESULT r = CallOldProcedure(hWnd, CB_ADDSTRING, wParam, (LPARAM)newtext.c_str());
				if((int)r >= 0)
					i->second.OldElements.insert(i->second.OldElements.begin() + (int)r, oldtext);
				return r;
			}
			else if(msg == CB_INSERTSTRING)
			{
				int pos = (int)wParam;
				const char16_t *oldtext = (const char16_t*)lParam;
				std::u16string newtext = ActPack ? ActPack->ReplaceIndentifiers(oldtext) : oldtext;
				LRESULT r = CallOldProcedure(hWnd, CB_INSERTSTRING, wParam, (LPARAM)newtext.c_str());
				if((int)r >= 0)
					i->second.OldElements.insert(i->second.OldElements.begin() + (int)r, oldtext);
				return r;
			}
			else if(msg == CB_DELETESTRING)
			{
				int pos = (int)wParam;
				LRESULT r = CallOldProcedure(hWnd, CB_DELETESTRING, wParam, 0);
				if((int)r >= 0)
					i->second.OldElements.erase(i->second.OldElements.begin() + (int)r);
				return r;
			}
			else if(msg == CB_RESETCONTENT)
			{
				i->second.OldElements.clear();
			}
		}
		return CallOldProcedure(hWnd, msg, wParam, lParam);
	}

public:
	ComboBoxSubclass() : CommonSubclass(L"combobox"), ActPack(0) {}

	void ChangeLanguage(const LanguagePack *p)
	{
		ActPack = p;
		for(auto i = Controls.begin(); i != Controls.end(); ++i)
		{
			int oldpos = (int)CallOldProcedure(i->first, CB_GETCURSEL, 0, 0);
			CallOldProcedure(i->first, WM_SETREDRAW, (WPARAM)false, 0);
			CallOldProcedure(i->first, CB_RESETCONTENT, 0, 0);

			auto v = i->second.OldElements;
			for(auto j = v.begin(); j != v.end(); ++j)
				CallOldProcedure(i->first, CB_ADDSTRING, 0, (LPARAM)(p ? p->ReplaceIndentifiers(j->c_str()).c_str() : j->c_str()));

			CallOldProcedure(i->first, WM_SETREDRAW, (WPARAM)true, 0);
			CallOldProcedure(i->first, CB_SETCURSEL, (WPARAM)oldpos, 0);
		}
	}

};

//////////////////////////////////////////////////////////////////////////////////////////////
// function hook simplifier
//////////////////////////////////////////////////////////////////////////////////////////

class FunctionRedirection
{
private:
	uintptr_t OrigFunc;
	uintptr_t NewFunc;
	uint8_t OrigCode[5];
	uint8_t NewCode[5];
	bool Enabled;

public:
	template<class F>
	FunctionRedirection(F &origfunc, F &newfunc)
	{
		OrigFunc = (uintptr_t)(&origfunc);
		NewFunc = (uintptr_t)(&newfunc);
		ReadProcessMemory(GetCurrentProcess(), origfunc, OrigCode, 5, 0);
		NewCode[0] = 0xE9;
		*(uintptr_t*)(&NewCode[1]) = NewFunc - OrigFunc - 5;
		Enable();
	}
	~FunctionRedirection()
	{
		Disable();
	}
	void Enable()
	{
		WriteProcessMemory(GetCurrentProcess(), (void*)OrigFunc, NewCode, 5, 0);
		Enabled = true;
	}
	void Disable()
	{
		WriteProcessMemory(GetCurrentProcess(), (void*)OrigFunc, OrigCode, 5, 0);
		Enabled = false;
	}
	bool isEnabled() const { return Enabled; }
};


//////////////////////////////////////////////////////////////////////////////////////////
// window menus
///////////////////////////////////////////////////////////////////////////////

class Menu
{
private:
	HMENU hMenu;
	std::vector<std::u16string> OldEntries;
	const LanguagePack *ActPack;

	void setMenuString(int pos, const wchar_t *str)
	{
		MENUITEMINFOW mi;
		mi.cbSize = sizeof(mi);
		mi.fMask = MIIM_STRING;
		mi.dwTypeData = const_cast<LPWSTR>(str);
		SetMenuItemInfoW(hMenu, pos, true, &mi);
	}

	void EnumEntries()
	{
		OldEntries.clear();
		int n = GetMenuItemCount(hMenu);
		for(int i = 0; i < n; ++i)
		{
			char16_t buff[256];
			if(GetMenuStringW(hMenu, i, (wchar_t*)buff, 256, MF_BYPOSITION) != 0)
			{
				OldEntries.push_back(buff);
				setMenuString(i, (const wchar_t*)(ActPack ? ActPack->ReplaceIndentifiers(buff).c_str() : buff));
			}
			else
				OldEntries.push_back(u"");
		}
	}

public:
	Menu(const LanguagePack *p, HMENU hhMenu) : ActPack(p), hMenu(hhMenu)
	{
		EnumEntries();
	}
	void ChangeLanguage(const LanguagePack *p)
	{
		ActPack = p;
		int j = 0;
		std::wstring n;
		for(auto i = OldEntries.begin(); i != OldEntries.end(); ++i, ++j)
		{
			setMenuString(j, (const wchar_t*)(p ? p->ReplaceIndentifiers(i->c_str()).c_str() : i->c_str()));
		}
	}
};

class MenuSubclass
{
private:
	static MenuSubclass *ActSubclass;

	std::map<HMENU, std::unique_ptr<Menu>> Menus;
	std::vector<HWND> WindowMenus;

	const LanguagePack *ActPack;

public:
	MenuSubclass() : ActPack(0)
	{
		ActSubclass = this;
	}
	~MenuSubclass()
	{
		ActSubclass = nullptr;
	}
	void RecursiveAddMenu(HMENU hMenu)
	{
		if(hMenu)
		{
			if(Menus.find(hMenu) == Menus.end())
			{
				std::unique_ptr<Menu> p(new Menu(ActPack, hMenu));
				Menus[hMenu].reset(p.release());

				int n = GetMenuItemCount(hMenu);
				for(int i = 0; i < n; ++i)
				{
					HMENU hSub = GetSubMenu(hMenu, i);
					if(hSub)
						RecursiveAddMenu(hSub);
				}
			}
		}
	}
	void RecursiveRemoveMenu(HMENU hMenu)
	{
		auto i = Menus.find(hMenu);
		if(i != Menus.end())
			Menus.erase(i);
		int n = GetMenuItemCount(hMenu);
		for(int i = 0; i < n; ++i)
		{
			HMENU hSub = GetSubMenu(hMenu, i);
			if(hSub)
				RecursiveRemoveMenu(hSub);
		}
	}
	void AddMenuWindow(HWND hWnd)
	{
		if(hWnd)
		{
			auto i = std::find(WindowMenus.begin(), WindowMenus.end(), hWnd);
			if(i == WindowMenus.end())
				WindowMenus.push_back(hWnd);
		}
	}
	void RemoveMenuWindow(HWND hWnd)
	{
		auto i = std::find(WindowMenus.begin(), WindowMenus.end(), hWnd);
		if(i != WindowMenus.end())
			WindowMenus.erase(i);
	}
	void AddWindowMenu(HWND hWnd)
	{
		HMENU hMenu = GetMenu(hWnd);
		if(hMenu)
		{
			RecursiveAddMenu(hMenu);
			AddMenuWindow(hWnd);
		}
	}
	void ChangeLanguage(const LanguagePack *p)
	{
		ActPack = p;
		for(auto i = Menus.begin(); i != Menus.end(); ++i)
			i->second->ChangeLanguage(p);
		for(auto i = WindowMenus.begin(); i != WindowMenus.end(); ++i)
			DrawMenuBar(*i);
	}

};

MenuSubclass *MenuSubclass::ActSubclass = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////
// function replacements
//////////////////////////////////////////////////////////////////////////////////////////

class FunctionReplacements
{
private:
	static FunctionReplacements *ActInst;
	const LanguagePack *ActPack;
	MenuSubclass NewMenus;
	FunctionRedirection CreateDialogParamARed, CreateDialogParamWRed;
	FunctionRedirection CreateDialogIndirectParamARed, CreateDialogIndirectParamWRed;
	FunctionRedirection CreateWindowExARed, CreateWindowExWRed;
	FunctionRedirection DestroyWindowRed;
	FunctionRedirection CreateMenuRed, DestroyMenuRed;
	FunctionRedirection LoadMenuARed, LoadMenuWRed;
	FunctionRedirection LoadMenuIndirectARed, LoadMenuIndirectWRed;
	FunctionRedirection SetMenuRed;
	FunctionRedirection DefWindowProcARed, DefWindowProcWRed;
	FunctionRedirection DefDlgProcARed, DefDlgProcWRed;

	void FlushWindow(HWND hWnd) //umltimately calls hooked DefWindowProc below
	{
		if(hWnd)
		{
			wchar_t b[512];
			GetWindowTextW(hWnd, b, 512);
			SetWindowTextW(hWnd, b);
		}
	}

	static HWND WINAPI MyCreateDialogParamA(HINSTANCE hInst, LPCSTR hTempl, HWND hParent, DLGPROC proc, LPARAM lParam)
	{
		ActInst->CreateDialogParamARed.Disable();
		HWND hWnd = CreateDialogParamA(hInst, hTempl, hParent, proc, lParam);
		ActInst->CreateDialogParamARed.Enable();
		ActInst->NewMenus.AddWindowMenu(hWnd);
		ActInst->FlushWindow(hWnd);
		return hWnd;
	}
	static HWND WINAPI MyCreateDialogParamW(HINSTANCE hInst, LPCWSTR hTempl, HWND hParent, DLGPROC proc, LPARAM lParam)
	{
		ActInst->CreateDialogParamWRed.Disable();
		HWND hWnd = CreateDialogParamW(hInst, hTempl, hParent, proc, lParam);
		ActInst->CreateDialogParamWRed.Enable();
		ActInst->NewMenus.AddWindowMenu(hWnd);
		ActInst->FlushWindow(hWnd);
		return hWnd;
	}
	static HWND WINAPI MyCreateDialogIndirectParamA(HINSTANCE hInst, LPCDLGTEMPLATEA hTempl, HWND hParent, DLGPROC proc, LPARAM lParam)
	{
		ActInst->CreateDialogIndirectParamARed.Disable();
		HWND hWnd = CreateDialogIndirectParamA(hInst, hTempl, hParent, proc, lParam);
		ActInst->CreateDialogIndirectParamARed.Enable();
		ActInst->NewMenus.AddWindowMenu(hWnd);
		ActInst->FlushWindow(hWnd);
		return hWnd;
	}
	static HWND WINAPI MyCreateDialogIndirectParamW(HINSTANCE hInst, LPCDLGTEMPLATEW hTempl, HWND hParent, DLGPROC proc, LPARAM lParam)
	{
		ActInst->CreateDialogIndirectParamWRed.Disable();
		HWND hWnd = CreateDialogIndirectParamW(hInst, hTempl, hParent, proc, lParam);
		ActInst->CreateDialogIndirectParamWRed.Enable();
		ActInst->NewMenus.AddWindowMenu(hWnd);
		ActInst->FlushWindow(hWnd);
		return hWnd;
	}
	static HWND WINAPI MyCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y,
		int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
	{
		ActInst->CreateWindowExARed.Disable();
		HWND hWnd = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		ActInst->CreateWindowExARed.Enable();
		ActInst->NewMenus.AddMenuWindow(hWnd);
		ActInst->FlushWindow(hWnd);
		return hWnd;
	}
	static HWND WINAPI MyCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y,
		int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
	{
		ActInst->CreateWindowExWRed.Disable();
		HWND hWnd = CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		ActInst->CreateWindowExWRed.Enable();
		ActInst->NewMenus.AddMenuWindow(hWnd);
		ActInst->FlushWindow(hWnd);
		return hWnd;
	}
	static BOOL WINAPI MyDestroyWindow(HWND hWnd)
	{
		HMENU hMenu = GetMenu(hWnd);
		ActInst->NewMenus.RecursiveRemoveMenu(hMenu);
		ActInst->NewMenus.RemoveMenuWindow(hWnd);
		ActInst->DestroyWindowRed.Disable();
		BOOL b = DestroyWindow(hWnd);
		ActInst->DestroyWindowRed.Enable();
		return b;
	}
	static HMENU WINAPI MyCreateMenu()
	{
		ActInst->CreateMenuRed.Disable();
		HMENU hMenu = CreateMenu();
		ActInst->CreateMenuRed.Enable();
		ActInst->NewMenus.RecursiveAddMenu(hMenu);
		return hMenu;
	}
	static BOOL WINAPI MyDestroyMenu(HMENU hMenu)
	{
		ActInst->DestroyMenuRed.Disable();
		BOOL b = DestroyMenu(hMenu);
		ActInst->DestroyMenuRed.Enable();
		ActInst->NewMenus.RecursiveRemoveMenu(hMenu);
		return b;
	}
	static HMENU WINAPI MyLoadMenuA(HINSTANCE hInst, LPCSTR lpMenu)
	{
		ActInst->LoadMenuARed.Disable();
		HMENU hMenu = LoadMenuA(hInst, lpMenu);
		ActInst->LoadMenuARed.Enable();
		ActInst->NewMenus.RecursiveAddMenu(hMenu);
		return hMenu;
	}
	static HMENU WINAPI MyLoadMenuW(HINSTANCE hInst, LPCWSTR lpMenu)
	{
		ActInst->LoadMenuWRed.Disable();
		HMENU hMenu = LoadMenuW(hInst, lpMenu);
		ActInst->LoadMenuWRed.Enable();
		ActInst->NewMenus.RecursiveAddMenu(hMenu);
		return hMenu;
	}
	static HMENU WINAPI MyLoadMenuIndirectA(const MENUTEMPLATEA *lpMenuTemplate)
	{
		ActInst->LoadMenuIndirectARed.Disable();
		HMENU hMenu = LoadMenuIndirectA(lpMenuTemplate);
		ActInst->LoadMenuIndirectARed.Enable();
		ActInst->NewMenus.RecursiveAddMenu(hMenu);
		return hMenu;
	}
	static HMENU WINAPI MyLoadMenuIndirectW(const MENUTEMPLATEW *lpMenuTemplate)
	{
		ActInst->LoadMenuIndirectWRed.Disable();
		HMENU hMenu = LoadMenuIndirectW(lpMenuTemplate);
		ActInst->LoadMenuIndirectWRed.Enable();
		ActInst->NewMenus.RecursiveAddMenu(hMenu);
		return hMenu;
	}
	static BOOL WINAPI MySetMenu(HWND hWnd, HMENU hMenu)
	{
		ActInst->SetMenuRed.Disable();
		BOOL b = SetMenu(hWnd, hMenu);
		ActInst->NewMenus.AddMenuWindow(hWnd);
		ActInst->NewMenus.RemoveMenuWindow(hWnd);
		ActInst->SetMenuRed.Enable();
		return b;
	}
	static LRESULT WINAPI MyDefWindowProcA(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		ActInst->DefWindowProcARed.Disable();
		LRESULT r;
		if(ActInst->ActPack && msg == WM_SETTEXT)
			r = DefWindowProcW(hWnd, WM_SETTEXT, 0, (LPARAM)(AsciiToUTF16((const char*)lParam).c_str()));
		else
			r = DefWindowProcA(hWnd, msg, wParam, lParam);
		ActInst->DefWindowProcARed.Enable();
		return r;
	}
	static LRESULT WINAPI MyDefWindowProcW(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		ActInst->DefWindowProcWRed.Disable();
		LRESULT r;
		if(ActInst->ActPack && msg == WM_SETTEXT)
		{
			std::u16string s = ActInst->ActPack->ReplaceIndentifiers((const char16_t*)lParam);
			r = DefWindowProcW(hWnd, WM_SETTEXT, 0, (LPARAM)s.c_str());
		}
		else
			r = DefWindowProcW(hWnd, msg, wParam, lParam);
		ActInst->DefWindowProcWRed.Enable();
		return r;
	}
	static LRESULT WINAPI MyDefDlgProcA(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		ActInst->DefDlgProcARed.Disable();
		LRESULT r;
		if(ActInst->ActPack && msg == WM_SETTEXT)
			r = DefDlgProcW(hWnd, WM_SETTEXT, 0, (LPARAM)AsciiToUTF16((const char*)lParam).c_str());
		else
			r = DefDlgProcA(hWnd, msg, wParam, lParam);
		ActInst->DefDlgProcARed.Enable();
		return r;
	}
	static LRESULT WINAPI MyDefDlgProcW(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		ActInst->DefDlgProcWRed.Disable();
		LRESULT r;
		if(ActInst->ActPack && msg == WM_SETTEXT)
		{
			std::u16string s = ActInst->ActPack->ReplaceIndentifiers((const char16_t*)lParam);
			r = DefDlgProcW(hWnd, WM_SETTEXT, 0, (LPARAM)s.c_str());
		}
		else
			r = DefDlgProcW(hWnd, msg, wParam, lParam);
		ActInst->DefDlgProcWRed.Enable();
		return r;
	}

public:
	FunctionReplacements() : ActPack(0),
		CreateDialogParamARed(CreateDialogParamA, MyCreateDialogParamA),
		CreateDialogParamWRed(CreateDialogParamW, MyCreateDialogParamW),
		CreateDialogIndirectParamARed(CreateDialogIndirectParamA, MyCreateDialogIndirectParamA),
		CreateDialogIndirectParamWRed(CreateDialogIndirectParamW, MyCreateDialogIndirectParamW),
		CreateWindowExARed(CreateWindowExA, MyCreateWindowExA),
		CreateWindowExWRed(CreateWindowExW, MyCreateWindowExW),
		DestroyWindowRed(DestroyWindow, MyDestroyWindow),
		CreateMenuRed(CreateMenu, MyCreateMenu),
		DestroyMenuRed(DestroyMenu, MyDestroyMenu),
		LoadMenuARed(LoadMenuA, MyLoadMenuA),
		LoadMenuWRed(LoadMenuW, MyLoadMenuW),
		LoadMenuIndirectARed(LoadMenuIndirectA, MyLoadMenuIndirectA),
		LoadMenuIndirectWRed(LoadMenuIndirectW, MyLoadMenuIndirectW),
		SetMenuRed(SetMenu, MySetMenu),
		DefWindowProcARed(DefWindowProcA, MyDefWindowProcA), DefWindowProcWRed(DefWindowProcW, MyDefWindowProcW),
		DefDlgProcARed(DefDlgProcA, MyDefDlgProcA), DefDlgProcWRed(DefDlgProcW, MyDefDlgProcW)
	{
		ActInst = this;
	}
	~FunctionReplacements()
	{
		ActInst = nullptr;
	}
	void ChangeLanguage(const LanguagePack *l)
	{
		ActPack = l;
		NewMenus.ChangeLanguage(l);
	}
};

FunctionReplacements *FunctionReplacements::ActInst = 0;

//////////////////////////////////////////////////////////////////////////////////////////////
// gui localizer
//////////////////////////////////////////////////////////////////////////////////////////

static std::unique_ptr<GuiLocalizer> GuiLocalizerInstance;

struct GuiLocalizer::ImplType
{
	StaticSubclass NewStatics;
	ButtonSubclass NewButtons;
	EditSubclass NewEdits;
	ListBoxSubclass NewListBoxes;
	MenuSubclass NewMenus;
	ListViewSubclass NewListViews;
	ComboBoxSubclass NewComboBoxes;
	FunctionReplacements NewFunctions;
};

GuiLocalizer &GuiLocalizer::getInstance()
{
	if(!GuiLocalizerInstance)
		throw std::runtime_error("[GuiLocalizer::getInstance] not initialized");
	return *GuiLocalizerInstance;
}
void GuiLocalizer::InitializeInstance()
{
	if(!GuiLocalizerInstance)
		GuiLocalizerInstance.reset(new GuiLocalizer);
}
void GuiLocalizer::ReleaseInstance()
{
	GuiLocalizerInstance.release();
}
GuiLocalizer::GuiLocalizer()
{
	Impl = new ImplType;
}
GuiLocalizer::~GuiLocalizer()
{
	delete Impl;
}
void GuiLocalizer::ChangeLanguage(const LanguagePack *p)
{
	ActPack = p;
	Impl->NewListViews.ChangeLanguage(p);
	Impl->NewStatics.ChangeLanguage(p);
	Impl->NewButtons.ChangeLanguage(p);
	Impl->NewEdits.ChangeLanguage(p);
	Impl->NewListBoxes.ChangeLanguage(p);
	Impl->NewComboBoxes.ChangeLanguage(p);
	Impl->NewFunctions.ChangeLanguage(p);
}


};
};
};
