#include "InventoryFunctions64.h"

class TESObjectREFR;
class TESForm;
class TESContainer;
class VMClassRegistry;
class BGSKeywordForm;
class BGSKeyword;

#include <vector>
#include <map>

typedef std::vector<InventoryEntryData*> ExtraDataVec;
typedef std::map<TESForm*, UInt32> ExtraContainerMap;


// static const RelocPtr<uintptr_t> s_ExtraPoisonVtbl(0x0153E4A0); // v2.0.4 (runtime 1.5.3)
// static const RelocPtr<uintptr_t> s_ExtraPoisonVtbl(0x015454B0); // v2.0.5 (runtime 1.5.16)
// static const RelocPtr<uintptr_t> s_ExtraPoisonVtbl(0x015454B0); // v2.0.6 (runtime 1.5.23)
// static const RelocPtr<uintptr_t> s_ExtraPoisonVtbl(0x015464B0); // v2.0.7 (runtime 1.5.39)
// static const RelocPtr<uintptr_t> s_ExtraPoisonVtbl(0x015464C0); // v2.0.8 (runtime 1.5.50)
// static const RelocPtr<uintptr_t> s_ExtraPoisonVtbl(0x015464C0); // v2.0.9 and 10 (runtime 1.5.53)
// static const RelocPtr<uintptr_t> s_ExtraPoisonVtbl(0x015464C0); // v2.0.11 and 12 (runtime 1.5.62)
//const RelocPtr<uintptr_t> s_ExtraPoisonVtbl(0x0152C470); // v2.0.13, 14 and 15 (runtime 1.5.73)
//const RelocPtr<uintptr_t> s_ExtraPoisonVtbl(0x0152C470); // v2.0.16 (runtime 1.5.80)
//const RelocPtr<uintptr_t> s_ExtraPoisonVtbl(0x0152C460); // v2.0.17, 18, 19 and 20 (runtime 1.5.97)
// ------------------------------------------------------------ AE Versions ----------------------------------------------------------------------
//const RelocPtr<uintptr_t> s_ExtraPoisonVtbl(0x01623E50); // v2.1.0 (runtime 1.6.317), v2.1.1 and 2 (runtime 1.6.318), v2.1.3 (runtime 1.6.323)
//const RelocPtr<uintptr_t> s_ExtraPoisonVtbl(0x01624E50); // v2.1.4 (runtime 1.6.342), v2.1.5 (runtime 1.6.353)
//const RelocPtr<uintptr_t> s_ExtraPoisonVtbl(0x01622EA0); // v2.2.0 (runtime 1.6.629)
//const RelocPtr<uintptr_t> s_ExtraPoisonVtbl(0x01622E90); // v2.2.1, 2 and 3 (runtime 1.6.640)
const RelocPtr<uintptr_t> s_ExtraPoisonVtbl(0x01766CF0); // v2.2.4 and 5 (runtime 1.6.1130)

ExtraPoison* ExtraPoisonCreate()
{
	ExtraPoison* xPoison = (ExtraPoison*)BSExtraData::Create(sizeof(ExtraPoison), s_ExtraPoisonVtbl.GetUIntPtr());
	xPoison->poison = NULL;
	xPoison->unk18 = 0;
	return xPoison;
}



bool EffectHasLevel(UInt32 effectLevel, SInt32 testLevel, SInt32 levelComparison)
{
	if (testLevel < 0)
		return true;

	bool levelPass = true;
	switch (levelComparison)
	{
	case -2:
		levelPass = effectLevel < testLevel;
		break;
	case -1:
		levelPass = effectLevel <= testLevel;
		break;
	case 0:
		levelPass = effectLevel == testLevel;
		break;
	case 1:
		levelPass = effectLevel >= testLevel;
		break;
	case 2:
		levelPass = effectLevel > testLevel;
		break;
	default:
		break;
	}
	return levelPass;
}

bool EffectIsMatch(MagicItem::EffectItem* pEI, BGSKeywordForm* pKeywords, BGSKeyword* thisKeyword, UInt32 schoolAV, SInt32 level, SInt32 levelComparison, SInt32 spellType)
{
	if (!pEI)
	{
		//_MESSAGE("Can't get Effect :(");
		return false;
	}

	UInt32 effSchool = pEI->mgef->school();
	// if we're testing for a Spell (spellType 0) of Novice level (level 0), then also enforce a School for the Effect, 
	// as most 'dummy' effects on Spells have no school and level 0
	if (spellType == 0 && level == 0 && effSchool >= 255)
	{
		//_MESSAGE("Testing for Novice level - skip un-schooled effect %s", pEI->mgef->fullName.GetName());
		return false;
	}

	bool keywordPass = !thisKeyword || (pKeywords && pKeywords->HasKeyword(thisKeyword));
	bool schoolPass = schoolAV == 255 || effSchool == schoolAV;
	bool levelPass = EffectHasLevel(pEI->mgef->level(), level, levelComparison);

	//_MESSAGE("InventoryFunctions_Papyrus::EffectIsMatch - effect %s: %d keyword(s) (%s), school %d (%s), level %d (%s)", pEI->mgef->fullName.GetName(), (pKeywords ? pKeywords->numKeywords : -1), keywordPass ? "pass" : "fail", effSchool, schoolPass ? "pass" : "fail", pEI->mgef->level(), levelPass ? "pass" : "fail");

	return keywordPass && schoolPass && levelPass;
}

bool SpellIsMatch(SpellItem* spell, BGSKeyword* thisKeyword, UInt32 schoolAV, SInt32 level, SInt32 levelComparison, SInt32 spellType)
{
	if (!spell)
	{
		return false;
	}

	if (spellType > -1 && spellType != spell->data.type)
	{
		return false;
	}

	BGSKeywordForm* pKeywords = NULL;
	pKeywords = DYNAMIC_CAST(spell, SpellItem, BGSKeywordForm);

	// if Level is being tested, only check the first Magic Effect, as that seems to be the one with an associated Level
	UInt32 effects = level < 0 ? spell->effectItemList.count : 1;
	for (UInt32 f = 0; f < effects; f++)
	{
		MagicItem::EffectItem* pEI = NULL;
		spell->effectItemList.GetNthItem(f, pEI);

		bool effectMatch = EffectIsMatch(pEI, pKeywords, thisKeyword, schoolAV, level, levelComparison, spellType);
		if (effectMatch)
		{
			//_MESSAGE("InventoryFunctions_Papyrus::EffectIsMatch - found on spell (%s), effect %d (%s): %d keyword(s), school %d, level %d", spell->fullName.GetName(), f, pEI->mgef->fullName.GetName(), (pKeywords ? pKeywords->numKeywords : -1), pEI->mgef->school(), pEI->mgef->level());
			return true;
		}
	}

	return false;
}

bool KeywordFormHasKeyword(BGSKeywordForm* Key, BGSKeyword* thisKeyword)
{
	if (Key)
	{
		for (UInt32 x = 0; x < Key->numKeywords; x++)
		{
			if (Key->keywords[x] == thisKeyword)
			{
				return true;
			}
		}
	}
	return false;
}


class ExtraContainerInfo
{
	ExtraDataVec	m_vec;
	ExtraContainerMap m_map;
public:
	ExtraContainerInfo(EntryDataList * entryList) : m_map(), m_vec()
	{
		m_vec.reserve(128);
		if (entryList)
		{
			entryList->Visit(*this);
		}
	}

	bool Accept(InventoryEntryData* data)
	{
		if (data)
		{
			m_vec.push_back(data);
			m_map[data->type] = m_vec.size() - 1;
		}
		return true;
	}

	bool IsValidEntry(TESContainer::Entry* pEntry, SInt32& numObjects)
	{
		if (pEntry)
		{
			numObjects = pEntry->count;
			TESForm* pForm = pEntry->form;

			if (DYNAMIC_CAST(pForm, TESForm, TESLevItem))
				return false;

			ExtraContainerMap::iterator it = m_map.find(pForm);
			ExtraContainerMap::iterator itEnd = m_map.end();
			if (it != itEnd)
			{
				UInt32 index = it->second;
				InventoryEntryData* pXData = m_vec[index];
				if (pXData)
				{
					numObjects += pXData->countDelta;
				}
				// clear the object from the vector so we don't bother to look for it
				// in the second step
				m_vec[index] = NULL;
			}

			if (numObjects > 0)
			{
				//if (IsConsoleMode())
				//{
				//	PrintItemType(pForm);
				//}
				return true;
			}
		}
		return false;
	}

	// returns the count of items left in the vector
	UInt32 CountItems()
	{
		UInt32 count = 0;
		ExtraDataVec::iterator itEnd = m_vec.end();
		ExtraDataVec::iterator it = m_vec.begin();
		while (it != itEnd)
		{
			InventoryEntryData* extraData = (*it);
			if (extraData && (extraData->countDelta > 0))
			{
				count++;
				//if (IsConsoleMode())
				//{
				//	PrintItemType(extraData->type);
				//}
			}
			++it;
		}
		return count;
	}

	UInt32 CountItemsBaseGoldValue(TESContainer* Cont, bool total)
	{
		UInt32 value = 0;
		ExtraDataVec::iterator itEnd = m_vec.end();
		ExtraDataVec::iterator it = m_vec.begin();
		while (it != itEnd)
		{
			InventoryEntryData* extraData = (*it);
			if (extraData && extraData->type && (extraData->countDelta > 0))
			{
				TESValueForm* pValue = DYNAMIC_CAST(extraData->type, TESForm, TESValueForm);
				if (pValue)
				{
					value += pValue->value * extraData->countDelta;
				}
				else
				{
					AlchemyItem* alchemyItem = DYNAMIC_CAST(extraData->type, TESForm, AlchemyItem);
					if (alchemyItem) {
						value += alchemyItem->itemData.value * extraData->countDelta;
					}
				}
			}
			++it;
		}
		return value;
	}

	UInt32 CountItemsWithKeyword(BGSKeyword* thisKeyword, TESContainer* Cont, bool total)
	{
		UInt32 count = 0;
		ExtraDataVec::iterator itEnd = m_vec.end();
		ExtraDataVec::iterator it = m_vec.begin();
		while (it != itEnd)
		{
			InventoryEntryData* extraData = (*it);
			if (extraData && (extraData->countDelta > 0))
			{
				TESForm* pForm = extraData->type;
				BGSKeywordForm* Key = NULL;
				Key = DYNAMIC_CAST(pForm, TESForm, BGSKeywordForm);
				if (KeywordFormHasKeyword(Key, thisKeyword))
				{
					if (total)
						count += extraData->countDelta;
					else
						count++;
				}
			}
			++it;
		}
		return count;
	}

	UInt32 CountItemsOfType(UInt32 type, TESContainer* Cont, bool total)
	{
		UInt32 count = 0;
		ExtraDataVec::iterator itEnd = m_vec.end();
		ExtraDataVec::iterator it = m_vec.begin();
		while (it != itEnd)
		{
			InventoryEntryData* extraData = (*it);
			if (extraData && (extraData->countDelta > 0))
			{
				TESForm* pForm = extraData->type;
				if (pForm->formType == type)
				{
					if (total)
						count += extraData->countDelta;
					else
						count++;
				}
			}
			++it;
		}
		return count;
	}

	InventoryEntryData* GetNth(UInt32 n, UInt32 count)
	{
		ExtraDataVec::iterator itEnd = m_vec.end();
		ExtraDataVec::iterator it = m_vec.begin();
		while (it != itEnd)
		{
			InventoryEntryData* extraData = (*it);
			if (extraData && (extraData->countDelta > 0))
			{
				if (count == n)
				{
					return extraData;
				}
				count++;
			}
			++it;
		}
		return NULL;
	}

	InventoryEntryData* GetNthWithKeyword(BGSKeyword* thisKeyword, UInt32 count, UInt32 KeywordIndex)
	{
		ExtraDataVec::iterator itEnd = m_vec.end();
		ExtraDataVec::iterator it = m_vec.begin();
		while (it != itEnd)
		{
			InventoryEntryData* extraData = (*it);
			if (extraData && (extraData->countDelta > 0))
			{
				BGSKeywordForm* Key = DYNAMIC_CAST(extraData->type, TESForm, BGSKeywordForm);
				if (KeywordFormHasKeyword(Key, thisKeyword))
				{
					if (count == KeywordIndex)
					{
						return extraData;
					}
					count++; // BB Moved this
				}
			}
			++it;
		}
		return NULL;
	}

	InventoryEntryData* GetNthOfType(UInt32 type, UInt32 count, UInt32 TypeIndex)
	{
		ExtraDataVec::iterator itEnd = m_vec.end();
		ExtraDataVec::iterator it = m_vec.begin();
		while (it != itEnd)
		{
			InventoryEntryData* extraData = (*it);
			if (extraData && (extraData->countDelta > 0))
			{
				if (extraData->type->formType == type)
				{
					if (count == TypeIndex)
					{
						return extraData;
					}
					count++; // BB Moved this
				}
			}
			++it;
		}
		return NULL;
	}
};


class ContainerCountIf
{
	ExtraContainerInfo& m_info;
public:
	ContainerCountIf(ExtraContainerInfo& info) : m_info(info) { }

	bool Accept(TESContainer::Entry* pEntry) const
	{
		SInt32 numObjects = 0; // not needed in this count
		return m_info.IsValidEntry(pEntry, numObjects);
	}
};

class ContainerTotalBaseValue
{
	ExtraContainerInfo& m_info;
	UInt32 m_totalBaseValue;
public:
	ContainerTotalBaseValue(ExtraContainerInfo& info) : m_info(info), m_totalBaseValue(0.0) { }

	bool Accept(TESContainer::Entry* pEntry)
	{
		SInt32 numObjects = 0;
		if (m_info.IsValidEntry(pEntry, numObjects)) {
			TESValueForm* pValue = DYNAMIC_CAST(pEntry->form, TESForm, TESValueForm);
			if (pValue)
			{
				m_totalBaseValue += pValue->value;
			}
			else
			{
				AlchemyItem* alchemyItem = DYNAMIC_CAST(pEntry->form, TESForm, AlchemyItem);
				if (alchemyItem) {
					m_totalBaseValue += alchemyItem->itemData.value;
				}
			}
		}

		return true;
	}

	float GetTotalBaseValue() { return m_totalBaseValue; }
};

class MyContainerFindNthWithKeyword
{
	ExtraContainerInfo& m_info;
	UInt32 m_findIndex;
	UInt32 m_curIndex;
	BGSKeyword* Keyword;
public:
	MyContainerFindNthWithKeyword(ExtraContainerInfo& info, UInt32 findIndex, BGSKeyword* thisKeyword) : m_info(info), m_findIndex(findIndex), m_curIndex(0), Keyword(thisKeyword) { }

	bool Accept(TESContainer::Entry* pEntry)
	{
		SInt32 numObjects = 0;
		if (m_info.IsValidEntry(pEntry, numObjects))
		{
			BGSKeywordForm* Key = DYNAMIC_CAST(pEntry->form, TESForm, BGSKeywordForm);
			if (KeywordFormHasKeyword(Key, Keyword))
			{
				if (m_curIndex == m_findIndex)
				{
					return true;
				}
				m_curIndex++; // BB Moved this
			}
		}
		return false;
	}

	UInt32 GetCurIdx() { return m_curIndex; }
};

class MyContainerFindNthOfType
{
	ExtraContainerInfo& m_info;
	UInt32 m_findIndex;
	UInt32 m_curIndex;
	UInt32 Type;
public:
	MyContainerFindNthOfType(ExtraContainerInfo& info, UInt32 findIndex, UInt32 type) : m_info(info), m_findIndex(findIndex), m_curIndex(0), Type(type) { }

	bool Accept(TESContainer::Entry* pEntry)
	{
		SInt32 numObjects = 0;
		if (m_info.IsValidEntry(pEntry, numObjects))
		{
			if (pEntry->form->formType == Type)
			{
				if (m_curIndex == m_findIndex)
				{
					return true;
				}
				m_curIndex++; // BB Moved this
			}
		}
		return false;
	}

	UInt32 GetCurIdx() { return m_curIndex; }
};


namespace InventoryFunctions_ReferenceUtils
{
	UInt32 SetPoison(TESForm * baseForm, BaseExtraList* extraData, AlchemyItem* poison, UInt32 value)
	{
		// Object must be a weapon
		if (!baseForm || baseForm->formType != TESObjectWEAP::kTypeID)
			return -1;

		ExtraPoison* extraPoison = static_cast<ExtraPoison*>(extraData->GetByType(kExtraData_Poison));
		if (extraPoison)
		{
			//_MESSAGE("InventoryFunctions_ReferenceUtils::SetPoison - already poisoned, returning");
			return -1;
		}

		ExtraPoison* newPoison = ExtraPoisonCreate();
		newPoison->poison = poison;
		newPoison->unk18 = value;
		extraData->Add(kExtraData_Poison, newPoison);

		//_MESSAGE("InventoryFunctions_ReferenceUtils::SetPoison - poisoned with %s, %d charges", newPoison->poison->fullName.GetName(), newPoison->unk18);
		return newPoison->unk18;
	}

	void RemovePoison(TESForm * baseForm, BaseExtraList* extraData)
	{
		// Object must be a weapon
		if (!baseForm || baseForm->formType != TESObjectWEAP::kTypeID)
			return;

		BSExtraData* extraDataData = extraData->GetByType(kExtraData_Poison);
		ExtraPoison* extraPoison = static_cast<ExtraPoison*>(extraDataData);
		if (!extraPoison)
		{
			//_MESSAGE("InventoryFunctions_ReferenceUtils::ClearPoison - wasn't poisoned anyway");
			return;
		}

		bool removed = extraData->Remove(kExtraData_Poison, extraDataData);
		//_MESSAGE("InventoryFunctions_ReferenceUtils::ClearPoison - %s", removed ? "worked" : "failed");
	}

	UInt32 GetPoisonCharges(TESForm * baseForm, BaseExtraList* extraData)
	{
		// Object must be a weapon
		if (!baseForm || baseForm->formType != TESObjectWEAP::kTypeID)
			return -1;

		ExtraPoison* extraPoison = static_cast<ExtraPoison*>(extraData->GetByType(kExtraData_Poison));
		if (!extraPoison)
		{
			//_MESSAGE("InventoryFunctions_ReferenceUtils::GetPoisonCharges - not poisoned");
			return -1;
		}

		//_MESSAGE("InventoryFunctions_ReferenceUtils::GetPoisonCharges - %d", extraPoison->unk18);
		return extraPoison->unk18;
	}

	UInt32 SetPoisonCharges(TESForm * baseForm, BaseExtraList* extraData, UInt32 value)
	{
		// Object must be a weapon
		if (!baseForm || baseForm->formType != TESObjectWEAP::kTypeID)
			return -1;

		ExtraPoison* extraPoison = static_cast<ExtraPoison*>(extraData->GetByType(kExtraData_Poison));
		if (!extraPoison)
		{
			//_MESSAGE("InventoryFunctions_ReferenceUtils::SetPoisonCharges - not poisoned");
			return -1;
		}

		//_MESSAGE("InventoryFunctions_ReferenceUtils::SetPoisonCharges - set to %d", value);
		extraPoison->unk18 = value;
		return extraPoison->unk18;
	}
}


namespace InventoryFunctions_Papyrus
{
	VMResultArray<SpellItem*> ActorGetSpells(StaticFunctionTag* base, Actor* actor, BGSKeyword* thisKeyword, BSFixedString school, SInt32 level, SInt32 levelComparison, bool searchBase)
	{
		VMResultArray<SpellItem*> result;
		UInt32 actorValue = ActorValueList::ResolveActorValueByName(school.data);

		if (!actor)
			return result;

		UInt32 spells = actor->addedSpells.Length();

		char buffer[33];
		sprintf_s(buffer, "%d", level);
		//_MESSAGE("InventoryFunctions_Papyrus::GetActorSpells - %d spells, looking for keyword %s, school %s (%d), level %s, comparison %d", spells, (!thisKeyword ? "(any)" : thisKeyword->keyword.Get()), actorValue == 255 ? "(any)" : school.data, actorValue, level < 0 ? "(any)" : buffer, levelComparison);

		for (UInt32 n = 0; n < spells; n++)
		{
			SpellItem* spell = actor->addedSpells.Get(n);
			if (SpellIsMatch(spell, thisKeyword, actorValue, level, levelComparison, 0))
			{
				//_MESSAGE("InventoryFunctions_Papyrus::GetActorSpells - found on spell %d (%s)", n, spell->fullName.GetName());
				result.push_back(spell);
			}
		}

		if (!searchBase)
		{
			return result;
		}

		TESNPC* npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		VMResultArray<SpellItem*> baseResults = ActorBaseGetSpells(base, npc, thisKeyword, school, level, levelComparison);
		for (UInt32 i = 0; i < baseResults.size(); i++)
		{
			result.push_back(baseResults.at(i));
		}

		return result;
	}

	VMResultArray<SpellItem*> ActorBaseGetSpells(StaticFunctionTag* base, TESNPC* thisNPC, BGSKeyword* thisKeyword, BSFixedString school, SInt32 level, SInt32 levelComparison)
	{
		VMResultArray<SpellItem*> result;
		UInt32 actorValue = ActorValueList::ResolveActorValueByName(school.data);

		if (!thisNPC)
			return result;

		UInt32 spells = thisNPC->spellList.GetSpellCount();

		char buffer[33];
		sprintf_s(buffer, "%d", level);
		//_MESSAGE("InventoryFunctions_Papyrus::GetActorBaseSpells - %s has %d spells, looking for keyword %s, school %s (%d), level %s, comparison %d", thisNPC->fullName.GetName(), spells, (!thisKeyword ? "(any)" : thisKeyword->keyword.Get()), actorValue == 255 ? "(any)" : school.data, actorValue, level < 0 ? "(any)" : buffer, levelComparison);
		for (UInt32 n = 0; n < spells; n++)
		{
			SpellItem* spell = thisNPC->spellList.GetNthSpell(n);
			if (SpellIsMatch(spell, thisKeyword, actorValue, level, levelComparison, 0))
			{
				//_MESSAGE("InventoryFunctions_Papyrus::GetActorBaseSpells - found on spell %d (%s)", n, spell->fullName.GetName());
				result.push_back(spell);
			}
		}

		return result;
	}


	VMResultArray<TESShout*> ActorBaseGetShouts(StaticFunctionTag* base, TESNPC* thisNPC, BGSKeyword* thisKeyword)
	{
		VMResultArray<TESShout*> result;
		UInt32 shouts = thisNPC->spellList.GetShoutCount();
		for (UInt32 n = 0; n < shouts; n++)
		{
			TESShout* shout = thisNPC->spellList.GetNthShout(n);
			for (UInt32 w = 0; w < (UInt32)shout->words->kNumWords; w++)
			{
				SpellItem* spell = shout->words[w].spell;
				BGSKeywordForm* pKeywords = NULL;
				pKeywords = DYNAMIC_CAST(spell, SpellItem, BGSKeywordForm);
				if (spell && pKeywords && pKeywords->HasKeyword(thisKeyword))
				{
					//_MESSAGE("InventoryFunctions_Papyrus::ActorBaseHasShout - found on shout %d (%s), word %d (%s), has %d keywords", n, shout->fullName.GetName(), w, spell->fullName.GetName(), pKeywords->numKeywords);
					result.push_back(shout);
				}
			}
		}

		return result;
	}


	UInt32 SetPoison(StaticFunctionTag* base, TESObjectREFR* object, AlchemyItem* poison, UInt32 value)
	{
		if (!object)
		{
			//_MESSAGE("InventoryFunctions_Papyrus::SetPoison - No object - returning");
			return -1;
		}

		return InventoryFunctions_ReferenceUtils::SetPoison(object->baseForm, &object->extraData, poison, value);
	}

	void RemovePoison(StaticFunctionTag* base, TESObjectREFR* object)
	{
		if (!object)
		{
			//_MESSAGE("InventoryFunctions_Papyrus::RemovePoison - No object - returning");
			return;
		}

		return InventoryFunctions_ReferenceUtils::RemovePoison(object->baseForm, &object->extraData);
	}

	UInt32 GetPoisonCharges(StaticFunctionTag* base, TESObjectREFR* object)
	{
		if (!object)
			return -1;

		return InventoryFunctions_ReferenceUtils::GetPoisonCharges(object->baseForm, &object->extraData);
	}

	UInt32 SetPoisonCharges(StaticFunctionTag* base, TESObjectREFR* object, UInt32 value)
	{
		if (!object)
			return -1;

		return InventoryFunctions_ReferenceUtils::SetPoisonCharges(object->baseForm, &object->extraData, value);
	}


	UInt32 WornObjectSetPoison(WORNOBJECT_PARAMS, AlchemyItem* poison, UInt32 value)
	{
		EquipData equipData = referenceUtils::ResolveEquippedObject(actor, weaponSlot, slotMask);
		if (equipData.pForm && equipData.pExtraData)
		{
			return InventoryFunctions_ReferenceUtils::SetPoison(equipData.pForm, equipData.pExtraData, poison, value);
		}
		return -1;
	}

	void WornObjectRemovePoison(WORNOBJECT_PARAMS)
	{
		EquipData equipData = referenceUtils::ResolveEquippedObject(actor, weaponSlot, slotMask);
		if (equipData.pForm && equipData.pExtraData)
		{
			InventoryFunctions_ReferenceUtils::RemovePoison(equipData.pForm, equipData.pExtraData);
		}
	}

	UInt32 WornObjectGetPoisonCharges(WORNOBJECT_PARAMS)
	{
		EquipData equipData = referenceUtils::ResolveEquippedObject(actor, weaponSlot, slotMask);
		if (equipData.pForm && equipData.pExtraData)
		{
			return InventoryFunctions_ReferenceUtils::GetPoisonCharges(equipData.pForm, equipData.pExtraData);
		}
		return -1;
	}

	UInt32 WornObjectSetPoisonCharges(WORNOBJECT_PARAMS, UInt32 value)
	{
		EquipData equipData = referenceUtils::ResolveEquippedObject(actor, weaponSlot, slotMask);
		if (equipData.pForm && equipData.pExtraData)
		{
			return InventoryFunctions_ReferenceUtils::SetPoisonCharges(equipData.pForm, equipData.pExtraData, value);
		}
		return -1;
	}


	template <class Op>
	UInt32 MyCountIfByKeyword(Op& op, TESObjectREFR* ObjRef, BGSKeyword* thisKeyword, bool total)
	{
		TESContainer* Cont = NULL;
		BGSKeywordForm* key = NULL;
		Cont = DYNAMIC_CAST(ObjRef->baseForm, TESForm, TESContainer);
		UInt32 count = 0;
		for (UInt32 n = 0; n < Cont->numEntries; n++)
		{
			TESContainer::Entry* pEntry = Cont->entries[n];
			if (pEntry && op.Accept(pEntry))
			{
				key = DYNAMIC_CAST(pEntry->form, TESForm, BGSKeywordForm);
				if (KeywordFormHasKeyword(key, thisKeyword))
				{
					if (total)
						count += pEntry->count;
					else
						count++;
				}
			}
		}
		return count;
	}

	template <class Op>
	UInt32 MyCountIfByType(Op& op, TESObjectREFR* ObjRef, UInt32 type, bool total)
	{
		TESContainer* Cont = NULL;
		Cont = DYNAMIC_CAST(ObjRef->baseForm, TESForm, TESContainer);
		UInt32 count = 0;
		for (UInt32 n = 0; n < Cont->numEntries; n++)
		{
			TESContainer::Entry* pEntry = Cont->entries[n];
			if (pEntry && op.Accept(pEntry))
			{
				if (pEntry->form->formType == type)
				{
					if (total)
						count += pEntry->count;
					else
						count++;
				}
			}
		}
		return count;
	}


	UInt32 GetTotalBaseGoldValue(StaticFunctionTag* base, TESObjectREFR* pContainerRef)
	{
		if (!pContainerRef)
			return 0;
		ExtraContainerChanges* pXContainerChanges = static_cast<ExtraContainerChanges*>(pContainerRef->extraData.GetByType(kExtraData_ContainerChanges));
		if (!pXContainerChanges)
			return 0;

		UInt32 value = 0;
		TESContainer* pContainer = NULL;
		TESForm* pBaseForm = pContainerRef->baseForm;
		if (pBaseForm)
		{
			pContainer = DYNAMIC_CAST(pBaseForm, TESForm, TESContainer);
		}
		if (!pContainer)
			return 0;

		ExtraContainerInfo info(pXContainerChanges ? pXContainerChanges->data->objList : NULL);
		
		// first walk the base container
		if (pContainer)
		{
			ContainerTotalBaseValue valueCounter(info);
			pContainer->Visit(valueCounter);
			value = valueCounter.GetTotalBaseValue();
		}

		// now sum the remaining value
		value += info.CountItemsBaseGoldValue(pContainer, true);

		return value;
	}

	UInt32 GetNumItemsWithKeyword(StaticFunctionTag* base, TESObjectREFR* pContainerRef, BGSKeyword* thisKeyword)
	{
		if (!pContainerRef)
			return 0;

		TESContainer* pContainer = NULL;
		TESForm* pBaseForm = pContainerRef->baseForm;
		if (pBaseForm)
		{
			pContainer = DYNAMIC_CAST(pBaseForm, TESForm, TESContainer);
		}
		if (!pContainer)
			return 0;

		UInt32 count = 0;

		ExtraContainerChanges* pXContainerChanges = static_cast<ExtraContainerChanges*>(pContainerRef->extraData.GetByType(kExtraData_ContainerChanges));
		ExtraContainerInfo info(pXContainerChanges ? pXContainerChanges->data->objList : NULL);


		// first walk the base container
		if (pContainer)
		{
			ContainerCountIf counter(info);
			count += MyCountIfByKeyword(counter, pContainerRef, thisKeyword, false);
		}

		// now count the remaining items
		count += info.CountItemsWithKeyword(thisKeyword, pContainer, false);

		return count;
	}

	UInt32 GetNumItemsOfType(StaticFunctionTag* base, TESObjectREFR* pContainerRef, UInt32 type)
	{
		if (!pContainerRef)
			return 0;

		TESContainer* pContainer = NULL;
		TESForm* pBaseForm = pContainerRef->baseForm;
		if (pBaseForm)
		{
			pContainer = DYNAMIC_CAST(pBaseForm, TESForm, TESContainer);
		}
		if (!pContainer)
			return 0;

		UInt32 count = 0;

		ExtraContainerChanges* pXContainerChanges = static_cast<ExtraContainerChanges*>(pContainerRef->extraData.GetByType(kExtraData_ContainerChanges));
		ExtraContainerInfo info(pXContainerChanges ? pXContainerChanges->data->objList : NULL);


		// first walk the base container
		if (pContainer)
		{
			ContainerCountIf counter(info);
			count += MyCountIfByType(counter, pContainerRef, type, false);
		}

		// now count the remaining items
		count += info.CountItemsOfType(type, pContainer, false);

		return count;
	}

	TESForm* GetNthFormWithKeyword(StaticFunctionTag* base, TESObjectREFR* pContainerRef, BGSKeyword* thisKeyword, UInt32 n)
	{
		if (!pContainerRef)
			return NULL;

		TESContainer* pContainer = NULL;
		TESForm* pBaseForm = pContainerRef->baseForm;
		if (pBaseForm)
		{
			pContainer = DYNAMIC_CAST(pBaseForm, TESForm, TESContainer);
		}
		if (!pContainer)
			return NULL;

		UInt32 count = 0;

		ExtraContainerChanges* pXContainerChanges = static_cast<ExtraContainerChanges*>(pContainerRef->extraData.GetByType(kExtraData_ContainerChanges));
		ExtraContainerInfo info(pXContainerChanges ? pXContainerChanges->data->objList : NULL);

		// first look in the base container
		if (pContainer)
		{
			MyContainerFindNthWithKeyword finder(info, n, thisKeyword);
			TESContainer::Entry* pFound = pContainer->Find(finder);
			if (pFound)
			{
				return pFound->form;
			}
			else
			{
				count = finder.GetCurIdx();
			}
		}

		// now walk the remaining items in the map
		InventoryEntryData* pEntryData = info.GetNthWithKeyword(thisKeyword, count, n);
		if (pEntryData)
		{
			return pEntryData->type;
		}
		return NULL;
	}

	TESForm* GetNthFormOfType(StaticFunctionTag* base, TESObjectREFR* pContainerRef, UInt32 type, UInt32 n)
	{
		if (!pContainerRef)
			return NULL;

		TESContainer* pContainer = NULL;
		TESForm* pBaseForm = pContainerRef->baseForm;
		if (pBaseForm)
		{
			pContainer = DYNAMIC_CAST(pBaseForm, TESForm, TESContainer);
		}
		if (!pContainer)
			return NULL;

		UInt32 count = 0;

		ExtraContainerChanges* pXContainerChanges = static_cast<ExtraContainerChanges*>(pContainerRef->extraData.GetByType(kExtraData_ContainerChanges));
		ExtraContainerInfo info(pXContainerChanges ? pXContainerChanges->data->objList : NULL);

		// first look in the base container
		if (pContainer)
		{
			MyContainerFindNthOfType finder(info, n, type);
			TESContainer::Entry* pFound = pContainer->Find(finder);
			if (pFound)
			{
				return pFound->form;
			}
			else
			{
				count = finder.GetCurIdx();
			}
		}

		// now walk the remaining items in the map
		InventoryEntryData* pEntryData = info.GetNthOfType(type, count, n);
		if (pEntryData)
		{
			return pEntryData->type;
		}
		return NULL;
	}


	void SetIsBolt(StaticFunctionTag* base, TESAmmo* thisAmmo, bool isBolt)
	{
		if (!thisAmmo)
		{
			return;
		}
		isBolt ? thisAmmo->settings.flags &= ~TESAmmo::kNotBolt : thisAmmo->settings.flags |= TESAmmo::kNotBolt;
	}

	void SetProjectile(StaticFunctionTag* base, TESAmmo* thisAmmo, BGSProjectile* projectile)
	{
		if (!thisAmmo)
		{
			return;
		}
		thisAmmo->settings.projectile = projectile;
	}

	void SetDamage(StaticFunctionTag* base, TESAmmo* thisAmmo, float damage)
	{
		if (!thisAmmo)
		{
			return;
		}
		thisAmmo->settings.damage = damage;
	}


	bool RegisterPapyrusFunctions(VMClassRegistry* registry)
	{
		registry->RegisterFunction(
			new NativeFunction6<StaticFunctionTag, VMResultArray<SpellItem*>, Actor*, BGSKeyword*, BSFixedString, SInt32, SInt32, bool>("ActorGetSpells", "_Q2C_Functions", InventoryFunctions_Papyrus::ActorGetSpells, registry));

		registry->RegisterFunction(
			new NativeFunction5<StaticFunctionTag, VMResultArray<SpellItem*>, TESNPC*, BGSKeyword*, BSFixedString, SInt32, SInt32>("ActorBaseGetSpells", "_Q2C_Functions", InventoryFunctions_Papyrus::ActorBaseGetSpells, registry));

		registry->RegisterFunction(
			new NativeFunction2<StaticFunctionTag, VMResultArray<TESShout*>, TESNPC*, BGSKeyword*>("ActorBaseGetShouts", "_Q2C_Functions", InventoryFunctions_Papyrus::ActorBaseGetShouts, registry));



		registry->RegisterFunction(
			new NativeFunction3<StaticFunctionTag, UInt32, TESObjectREFR*, AlchemyItem*, UInt32>("SetPoison", "_Q2C_Functions", InventoryFunctions_Papyrus::SetPoison, registry));

		registry->RegisterFunction(
			new NativeFunction1<StaticFunctionTag, void, TESObjectREFR*>("RemovePoison", "_Q2C_Functions", InventoryFunctions_Papyrus::RemovePoison, registry));

		registry->RegisterFunction(
			new NativeFunction1<StaticFunctionTag, UInt32, TESObjectREFR*>("GetPoisonCharges", "_Q2C_Functions", InventoryFunctions_Papyrus::GetPoisonCharges, registry));

		registry->RegisterFunction(
			new NativeFunction2<StaticFunctionTag, UInt32, TESObjectREFR*, UInt32>("SetPoisonCharges", "_Q2C_Functions", InventoryFunctions_Papyrus::SetPoisonCharges, registry));


		registry->RegisterFunction(
			new NativeFunction5<StaticFunctionTag, UInt32, WORNOBJECT_TEMPLATE, AlchemyItem*, UInt32>("WornObjectSetPoison", "_Q2C_Functions", InventoryFunctions_Papyrus::WornObjectSetPoison, registry));

		registry->RegisterFunction(
			new NativeFunction3<StaticFunctionTag, void, WORNOBJECT_TEMPLATE>("WornObjectRemovePoison", "_Q2C_Functions", InventoryFunctions_Papyrus::WornObjectRemovePoison, registry));

		registry->RegisterFunction(
			new NativeFunction3<StaticFunctionTag, UInt32, WORNOBJECT_TEMPLATE>("WornObjectGetPoisonCharges", "_Q2C_Functions", InventoryFunctions_Papyrus::WornObjectGetPoisonCharges, registry));

		registry->RegisterFunction(
			new NativeFunction4<StaticFunctionTag, UInt32, WORNOBJECT_TEMPLATE, UInt32>("WornObjectSetPoisonCharges", "_Q2C_Functions", InventoryFunctions_Papyrus::WornObjectSetPoisonCharges, registry));


		registry->RegisterFunction(
			new NativeFunction2<StaticFunctionTag, UInt32, TESObjectREFR*, BGSKeyword*>("GetNumItemsWithKeyword", "_Q2C_Functions", InventoryFunctions_Papyrus::GetNumItemsWithKeyword, registry));

		registry->RegisterFunction(
			new NativeFunction3<StaticFunctionTag, TESForm*, TESObjectREFR*, BGSKeyword*, UInt32>("GetNthFormWithKeyword", "_Q2C_Functions", InventoryFunctions_Papyrus::GetNthFormWithKeyword, registry));

		registry->RegisterFunction(
			new NativeFunction2<StaticFunctionTag, UInt32, TESObjectREFR*, UInt32>("GetNumItemsOfType", "_Q2C_Functions", InventoryFunctions_Papyrus::GetNumItemsOfType, registry));

		registry->RegisterFunction(
			new NativeFunction3<StaticFunctionTag, TESForm*, TESObjectREFR*, UInt32, UInt32>("GetNthFormOfType", "_Q2C_Functions", InventoryFunctions_Papyrus::GetNthFormOfType, registry));


		registry->RegisterFunction(
			new NativeFunction1<StaticFunctionTag, UInt32, TESObjectREFR*>("GetTotalBaseGoldValue", "_Q2C_Functions", InventoryFunctions_Papyrus::GetTotalBaseGoldValue, registry));


		registry->RegisterFunction(
			new NativeFunction2<StaticFunctionTag, void, TESAmmo*, bool>("SetIsBolt", "_Q2C_Functions", InventoryFunctions_Papyrus::SetIsBolt, registry));

		registry->RegisterFunction(
			new NativeFunction2<StaticFunctionTag, void, TESAmmo*, BGSProjectile*>("SetProjectile", "_Q2C_Functions", InventoryFunctions_Papyrus::SetProjectile, registry));

		registry->RegisterFunction(
			new NativeFunction2<StaticFunctionTag, void, TESAmmo*, float>("SetDamage", "_Q2C_Functions", InventoryFunctions_Papyrus::SetDamage, registry));


		return true;
	}
}
