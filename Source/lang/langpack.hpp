#pragma once
#include <string>
#include <map>
#include <istream>
#include <list>
#include <vector>
#include <memory>
#include <cstdint>

// I do not want to include windows.h here as it is fairly big

namespace YFW{namespace Windows{namespace Locale
{

/////////////////////////////////////////////////////////////
// class language pack
// manages a language pack
/////////////////////////////////////////////////////

class LanguagePack
{
private:
	std::map<std::u16string, std::u16string> Identifiers;
	std::u16string Name;

public:
	LanguagePack(const char16_t *name) : Name(name) {}
	LanguagePack() {}
	~LanguagePack() {}

	//load pack from character stream
	void LoadPack(std::basic_istream<char32_t> &s);

	//load pack from file, returns false when file not found
	bool LoadFilePack(const char16_t *filename);

	//load pack from resource, returns false when resource not found
	bool LoadResourcePack(const char16_t *resourcename, const char16_t *resourcetype);
	bool LoadResourcePack(int resourceid, const char16_t *resourcetype);

	//get text by identifier, empty string when identifer not found
	const char16_t *getString(const char16_t *id) const;

	//get/set name of this pack
	void setName(const char16_t *name) { Name = name; }
	const char16_t *getName() const { return Name.c_str(); }

	//sets a string or add it when not already existing
	void setString(const char16_t *identifier, const char16_t *string) { Identifiers[identifier] = string; }

	//replaces '$!identifier$' with corresponding text
	std::u16string ReplaceIndentifiers(const char16_t *txt) const;
};

/////////////////////////////////////////////////////////////
// class language pack system
// loads and manages language packs
/////////////////////////////////////////////////////

class LanguagePackSystem
{
private:
	std::vector<std::unique_ptr<LanguagePack>> Packs;
	std::vector<std::u16string> AvailablePacks;
	std::u16string Path, Prefix, Suffix;
	LanguagePack *getPack(const char16_t *name);

	void operator=(const LanguagePackSystem&) {}
	LanguagePackSystem(const LanguagePackSystem&) {}

public:
	LanguagePackSystem() {}
	~LanguagePackSystem() {}

	//sets language pack location in 'path', beginning with 'prefix', ending with 'suffix'
	//the part between is taken as the name of the pack
	//e.g. prefix:"lang_", suffix:".txt" matches the file 'lang_English.txt', where 'English' is taken as the name
	void setLocation(const char16_t *path, const char16_t *prefix, const char16_t *suffix);

	//get all packs' names
	const std::vector<std::u16string> &getAvailablePacks() const { return AvailablePacks; }

	//loads pack file by name or returns already loaded pack (case-sensitive!)
	//returns nullptr when not found
	LanguagePack *RequestPack(const char16_t *name);

	//add a pack to list or replace when already existing
	void AddPack(const LanguagePack &p);

	//insert empty pack
	LanguagePack &AddPack();
};

////////////////////////////////////////////////////////////////////////
// GuiLocalizer
// globaly localizes Gui
////////////////////////////////////////////////////////////////////

class GuiLocalizer
{
	friend std::unique_ptr<GuiLocalizer>;
	friend std::default_delete<GuiLocalizer>;
private:
	GuiLocalizer();
	GuiLocalizer(const GuiLocalizer&) {}
	void operator=(const GuiLocalizer&) {}
	~GuiLocalizer();

	struct ImplType;
	ImplType *Impl;

	const LanguagePack *ActPack;

public:
	static void InitializeInstance();
	static void ReleaseInstance();
	static GuiLocalizer &getInstance();

	//pass null to reset to original contents
	void ChangeLanguage(const LanguagePack *p);
};

};};};