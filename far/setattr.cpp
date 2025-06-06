﻿/*
setattr.cpp

Установка атрибутов файлов
*/
/*
Copyright © 1996 Eugene Roshal
Copyright © 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// BUGBUG
#include "platform.headers.hpp"

// Self:
#include "setattr.hpp"

// Internal:
#include "flink.hpp"
#include "dialog.hpp"
#include "scantree.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "ctrlobj.hpp"
#include "constitle.hpp"
#include "taskbar.hpp"
#include "config.hpp"
#include "datetime.hpp"
#include "fileattr.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "network.hpp"
#include "fileowner.hpp"
#include "wakeful.hpp"
#include "uuids.far.dialogs.hpp"
#include "plugins.hpp"
#include "imports.hpp"
#include "lang.hpp"
#include "locale.hpp"
#include "string_utils.hpp"
#include "global.hpp"
#include "stddlg.hpp"
#include "FarDlgBuilder.hpp"
#include "cvtname.hpp"
#include "log.hpp"
#include "scrbuf.hpp"
#include "interf.hpp"

// Platform:
#include "platform.hpp"
#include "platform.env.hpp"
#include "platform.fs.hpp"

// Common:
#include "common/view/zip.hpp"

// External:
#include "format.hpp"

//----------------------------------------------------------------------------

enum SETATTRDLG
{
	SA_DOUBLEBOX,
	SA_TEXT_LABEL,
	SA_TEXT_NAME,
	SA_COMBO_HARDLINK,
	SA_TEXT_REPARSE_POINT,
	SA_EDIT_REPARSE_POINT,
	SA_COMBO_REPARSE_POINT,
	SA_TEXT_DEVICE,
	SA_EDIT_DEVICE,
	SA_SEPARATOR_AFTER_HEADER,

	SA_ATTR_FIRST,
	SA_CHECKBOX_READONLY = SA_ATTR_FIRST,
	SA_CHECKBOX_ARCHIVE,
	SA_CHECKBOX_HIDDEN,
	SA_CHECKBOX_SYSTEM,
	SA_CHECKBOX_COMPRESSED,
	SA_CHECKBOX_ENCRYPTED,
	SA_CHECKBOX_NOTINDEXED,
	SA_CHECKBOX_SPARSE,
	SA_CHECKBOX_TEMP,
	SA_CHECKBOX_OFFLINE,
	SA_ATTR_LAST = SA_CHECKBOX_OFFLINE,
	SA_BUTTON_ADVANCED,

	SA_TEXT_DATETIME,
	SA_TEXT_TITLEDATE,
	SA_TEXT_TITLETIME,
	SA_TEXT_LASTWRITE,
	SA_EDIT_WDATE,
	SA_EDIT_WTIME,
	SA_TEXT_CREATION,
	SA_EDIT_CDATE,
	SA_EDIT_CTIME,
	SA_TEXT_LASTACCESS,
	SA_EDIT_ADATE,
	SA_EDIT_ATIME,
	SA_TEXT_CHANGE,
	SA_EDIT_XDATE,
	SA_EDIT_XTIME,
	SA_BUTTON_ORIGINAL,
	SA_BUTTON_CURRENT,
	SA_BUTTON_BLANK,

	SA_TEXT_OWNER,
	SA_EDIT_OWNER,

	SA_SEPARATOR_BEFORE_SUBFOLODERS,
	SA_CHECKBOX_SUBFOLDERS,

	SA_SEPARATOR_BEFORE_MAIN_BUTTONS,
	SA_BUTTON_SET,
	SA_BUTTON_SYSTEMDLG,
	SA_BUTTON_CANCEL,

	SA_SEPARATOR_VERTICAL,
	SA_SEPARATOR_AFTER_TIME,

	SA_COUNT
};

constexpr size_t main_attributes_count = SA_ATTR_LAST - SA_ATTR_FIRST + 1;

enum advanced_attributes
{
	SA_ADVANCED_ATTRIBUTE_FIRST = 1,

	SA_CHECKBOX_REPARSEPOINT = SA_ADVANCED_ATTRIBUTE_FIRST,
	SA_CHECKBOX_VIRTUAL,
	SA_CHECKBOX_INTEGRITY_STREAM,
	SA_CHECKBOX_NO_SCRUB_DATA,
	SA_CHECKBOX_PINNED,
	SA_CHECKBOX_UNPINNED,
	SA_CHECKBOX_RECALL_ON_OPEN,
	SA_CHECKBOX_RECALL_ON_DATA_ACCESS,
	SA_CHECKBOX_STRICTLY_SEQUENTIAL,
	SA_CHECKBOX_DEVICE,

	SA_ADVANCED_ATTRIBUTE_LAST = SA_CHECKBOX_DEVICE,
};

constexpr size_t advanced_attributes_count = SA_ADVANCED_ATTRIBUTE_LAST - SA_ADVANCED_ATTRIBUTE_FIRST + 1;

enum DIALOGMODE
{
	MODE_FILE,
	MODE_FOLDER,
	MODE_MULTIPLE,
};

static const struct
{
	int Id;
	os::fs::attributes Attribute;
	lng LngId;
}
AttributeMap[]
{
	{ SA_CHECKBOX_READONLY,                     FILE_ATTRIBUTE_READONLY,              lng::MSetAttrReadOnly,           },
	{ SA_CHECKBOX_ARCHIVE,                      FILE_ATTRIBUTE_ARCHIVE,               lng::MSetAttrArchive,            },
	{ SA_CHECKBOX_HIDDEN,                       FILE_ATTRIBUTE_HIDDEN,                lng::MSetAttrHidden,             },
	{ SA_CHECKBOX_SYSTEM,                       FILE_ATTRIBUTE_SYSTEM,                lng::MSetAttrSystem,             },
	{ SA_CHECKBOX_COMPRESSED,                   FILE_ATTRIBUTE_COMPRESSED,            lng::MSetAttrCompressed,         },
	{ SA_CHECKBOX_ENCRYPTED,                    FILE_ATTRIBUTE_ENCRYPTED,             lng::MSetAttrEncrypted,          },
	{ SA_CHECKBOX_NOTINDEXED,                   FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,   lng::MSetAttrNotIndexed,         },
	{ SA_CHECKBOX_SPARSE,                       FILE_ATTRIBUTE_SPARSE_FILE,           lng::MSetAttrSparse,             },
	{ SA_CHECKBOX_TEMP,                         FILE_ATTRIBUTE_TEMPORARY,             lng::MSetAttrTemporary,          },
	{ SA_CHECKBOX_OFFLINE,                      FILE_ATTRIBUTE_OFFLINE,               lng::MSetAttrOffline,            },

	{ SA_CHECKBOX_REPARSEPOINT,                 FILE_ATTRIBUTE_REPARSE_POINT,         lng::MSetAttrReparsePoint,       },
	{ SA_CHECKBOX_VIRTUAL,                      FILE_ATTRIBUTE_VIRTUAL,               lng::MSetAttrVirtual,            },
	{ SA_CHECKBOX_INTEGRITY_STREAM,             FILE_ATTRIBUTE_INTEGRITY_STREAM,      lng::MSetAttrIntegrityStream,    },
	{ SA_CHECKBOX_NO_SCRUB_DATA,                FILE_ATTRIBUTE_NO_SCRUB_DATA,         lng::MSetAttrNoScrubData,        },
	{ SA_CHECKBOX_PINNED,                       FILE_ATTRIBUTE_PINNED,                lng::MSetAttrPinned,             },
	{ SA_CHECKBOX_UNPINNED,                     FILE_ATTRIBUTE_UNPINNED,              lng::MSetAttrUnpinned,           },
	{ SA_CHECKBOX_RECALL_ON_OPEN,               FILE_ATTRIBUTE_RECALL_ON_OPEN,        lng::MSetAttrRecallOnOpen,       },
	{ SA_CHECKBOX_RECALL_ON_DATA_ACCESS,        FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS, lng::MSetAttrRecallOnDataAccess, },
	{ SA_CHECKBOX_STRICTLY_SEQUENTIAL,          FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL,   lng::MSetAttrStrictlySequential, },
	{ SA_CHECKBOX_DEVICE,                       FILE_ATTRIBUTE_DEVICE,                lng::MSetAttrDevice,             },
};

static_assert(std::size(AttributeMap) == main_attributes_count + advanced_attributes_count);


static const struct time_map
{
	SETATTRDLG DateId;
	SETATTRDLG TimeId;
	os::chrono::time_point os::fs::find_data::* Accessor;
}
TimeMap[]
{
	{SA_EDIT_WDATE, SA_EDIT_WTIME, &os::fs::find_data::LastWriteTime  },
	{SA_EDIT_CDATE, SA_EDIT_CTIME, &os::fs::find_data::CreationTime   },
	{SA_EDIT_ADATE, SA_EDIT_ATIME, &os::fs::find_data::LastAccessTime },
	{SA_EDIT_XDATE, SA_EDIT_XTIME, &os::fs::find_data::ChangeTime     },
};

static int label_to_time_map_index(int Id)
{
	static_assert(std::size(TimeMap) == 4);

	switch (Id)
	{
	case SA_TEXT_LASTWRITE:    return 0;
	case SA_TEXT_CREATION:     return 1;
	case SA_TEXT_LASTACCESS:   return 2;
	case SA_TEXT_CHANGE:       return 3;
	default:
		std::unreachable();
	}
}

struct SetAttrDlgParam
{
	bool Plugin;
	DIALOGMODE DialogMode;
	string strSelName;

	struct
	{
		FARDIALOGITEMFLAGS Flags;
		int InitialValue;
		int CurrentValue;
		bool ChangedManually;
	}
	Attributes[std::size(AttributeMap)];

	struct
	{
		struct
		{
			string InitialValue;
			bool ChangedManually;
		}
		Date, Time;
	}
	Times[std::size(TimeMap)];

	struct
	{
		string InitialValue;
		bool ChangedManually;
	}
	Owner;
};

static auto time_point_to_string(os::chrono::time_point const TimePoint)
{
	return time_point_to_string(TimePoint, 16, 2);
}

static void set_date_or_time(Dialog* const Dlg, int const Id, string const& Value, bool const MakeUnchanged)
{
	Dlg->SendMessage(DM_SETTEXTPTR, Id, UNSAFE_CSTR(Value));
	Dlg->SendMessage(DM_EDITUNCHANGEDFLAG, Id, ToPtr(MakeUnchanged));
}

static void set_dates_and_times(Dialog* const Dlg, const time_map& TimeMapEntry, std::optional<os::chrono::time_point> const TimePoint, bool const MakeUnchanged)
{
	string Date, Time;

	if (TimePoint)
	{
		std::tie(Date, Time) = time_point_to_string(*TimePoint);
	}

	set_date_or_time(Dlg, TimeMapEntry.DateId, Date, MakeUnchanged);
	set_date_or_time(Dlg, TimeMapEntry.TimeId, Time, MakeUnchanged);
}

static void AdvancedAttributesDialog(SetAttrDlgParam& DlgParam)
{
	DialogBuilder Builder(lng::MSetAttrTitle, {}, [](Dialog* Dlg, intptr_t Msg, intptr_t Param1, void* Param2) -> intptr_t
	{
		switch (Msg)
		{
		case DN_BTNCLICK:
			// only remove or keep, not set
			if (Param1 == SA_CHECKBOX_REPARSEPOINT && std::bit_cast<intptr_t>(Param2) == BSTATE_CHECKED)
				return false;

			break;
		}
		return Dlg->DefProc(Msg, Param1, Param2);
	});

	int SavedState[advanced_attributes_count];

	for (const auto i: std::views::iota(0uz, advanced_attributes_count))
	{
		const auto AbsoluteIndex = main_attributes_count + i;
		auto& Attr = DlgParam.Attributes[main_attributes_count + i];
		SavedState[i] = Attr.CurrentValue;
		Builder.AddCheckbox(AttributeMap[AbsoluteIndex].LngId, Attr.CurrentValue, 0, flags::check_one(Attr.Flags, DIF_3STATE));
	}

	Builder.AddOKCancel();

	if (!Builder.ShowDialog())
		return;

	for (const auto i: std::views::iota(0uz, advanced_attributes_count))
	{
		auto& Attr = DlgParam.Attributes[main_attributes_count + i];

		if (Attr.CurrentValue != SavedState[i])
			Attr.ChangedManually = true;
	}
}

static intptr_t SetAttrDlgProc(Dialog* Dlg,intptr_t Msg,intptr_t Param1,void* Param2)
{
	auto& DlgParam = edit_as<SetAttrDlgParam>(Dlg->SendMessage(DM_GETDLGDATA, 0, nullptr));

	switch (Msg)
	{
	case DN_BTNCLICK:
		if (Param1 >= SA_ATTR_FIRST && Param1 <= SA_ATTR_LAST)
		{
			DlgParam.Attributes[Param1 - SA_ATTR_FIRST].CurrentValue = static_cast<FARCHECKEDSTATE>(std::bit_cast<intptr_t>(Param2));
			DlgParam.Attributes[Param1 - SA_ATTR_FIRST].ChangedManually = true;

			// Compressed / Encrypted are mutually exclusive
			if ((Param1 == SA_CHECKBOX_COMPRESSED || Param1 == SA_CHECKBOX_ENCRYPTED) &&
				static_cast<FARCHECKEDSTATE>(std::bit_cast<intptr_t>(Param2)) == BSTATE_CHECKED)
			{
				const auto OtherId = Param1 == SA_CHECKBOX_COMPRESSED? SA_CHECKBOX_ENCRYPTED : SA_CHECKBOX_COMPRESSED;

				if (static_cast<FARCHECKEDSTATE>(Dlg->SendMessage(DM_GETCHECK, OtherId, nullptr)) != BSTATE_UNCHECKED)
				{
					DlgParam.Attributes[OtherId - SA_ATTR_FIRST].CurrentValue = BSTATE_UNCHECKED;
					Dlg->SendMessage(DM_SETCHECK, OtherId, ToPtr(BSTATE_UNCHECKED));
				}
			}
		}
		else if (Param1 == SA_BUTTON_ADVANCED)
		{
			AdvancedAttributesDialog(DlgParam);
			return true;
		}
		else if (Param1 == SA_CHECKBOX_SUBFOLDERS)
		{
			// этот кусок всегда работает если есть хотя бы одна папка
			// иначе SA_CHECKBOX_SUBFOLDERS недоступен и всегда снят.
			const auto SubfoldersState = static_cast<FARCHECKEDSTATE>(std::bit_cast<intptr_t>(Param2));

			SCOPED_ACTION(Dialog::suppress_redraw)(Dlg);

			// Works in 2 modes: single directory or multiple selection
			for(auto& i: DlgParam.Attributes)
			{
				if (SubfoldersState == BSTATE_UNCHECKED)
				{
					if (DlgParam.DialogMode == MODE_FOLDER)
						i.Flags &= ~DIF_3STATE;

					if (!i.ChangedManually)
						i.CurrentValue = i.InitialValue;
				}
				else
				{
					i.Flags |= DIF_3STATE;

					if (!i.ChangedManually)
						i.CurrentValue = BSTATE_3STATE;
				}
			}

			for (const auto i: std::views::iota(SA_ATTR_FIRST + 0, SA_ATTR_LAST + 1))
			{
				const auto& Attr = DlgParam.Attributes[i - SA_ATTR_FIRST];
				Dlg->SendMessage(DM_SET3STATE, i, ToPtr((Attr.Flags & DIF_3STATE) != 0));
				Dlg->SendMessage(DM_SETCHECK, i, ToPtr(Attr.CurrentValue));
			}

			if (DlgParam.DialogMode == MODE_FOLDER)
			{
				for (const auto& [i, State]: zip(TimeMap, DlgParam.Times))
				{
					const auto process = [&](int const Id, auto& Component)
					{
						if (!Component.ChangedManually)
						{
							set_date_or_time(Dlg, Id, SubfoldersState == BSTATE_UNCHECKED? Component.InitialValue : L""s, true);
							Component.ChangedManually = false;
						}
					};

					process(i.DateId, State.Date);
					process(i.TimeId, State.Time);
				}
			}

			if (!DlgParam.Owner.ChangedManually)
			{
				Dlg->SendMessage(DM_SETTEXTPTR, SA_EDIT_OWNER, SubfoldersState == BSTATE_UNCHECKED? UNSAFE_CSTR(DlgParam.Owner.InitialValue) : nullptr);
				DlgParam.Owner.ChangedManually = false;
			}
		}
		// Set Original? / Set All? / Clear All?
		else if (Param1 == SA_BUTTON_ORIGINAL)
		{
			SCOPED_ACTION(Dialog::suppress_redraw)(Dlg);

			for (const auto& [i, State]: zip(TimeMap, DlgParam.Times))
			{
				const auto process = [&](int const Id, auto& Component)
				{
					set_date_or_time(Dlg, Id, Component.InitialValue, true);
					Component.ChangedManually = false;
				};

				process(i.DateId, State.Date);
				process(i.TimeId, State.Time);
			}

			Dlg->SendMessage(DM_SETFOCUS, SA_EDIT_WDATE, nullptr);
		}
		else if (Param1 == SA_BUTTON_CURRENT || Param1 == SA_BUTTON_BLANK)
		{
			std::optional<os::chrono::time_point> Time;
			if(Param1 == SA_BUTTON_CURRENT)
			{
				Time = os::chrono::nt_clock::now();
			}

			SCOPED_ACTION(Dialog::suppress_redraw)(Dlg);

			for (const auto& i: TimeMap)
			{
				set_dates_and_times(Dlg, i, Time, false);
			}

			Dlg->SendMessage(DM_SETFOCUS, SA_EDIT_WDATE, nullptr);
		}

		break;

	//BUGBUG: DefDlgProc вызывается дважды, второй раз Param1 может быть другим.
	case DN_CONTROLINPUT:
		{
			if (Param1 != SA_TEXT_LASTWRITE && Param1 != SA_TEXT_CREATION && Param1 != SA_TEXT_LASTACCESS && Param1 != SA_TEXT_CHANGE)
				break;

			const auto& Record = *static_cast<INPUT_RECORD const*>(Param2);
			if (Record.EventType != MOUSE_EVENT)
				break;

			SCOPED_ACTION(Dialog::suppress_redraw)(Dlg);

			if (Record.Event.MouseEvent.dwEventFlags == DOUBLE_CLICK)
				set_dates_and_times(Dlg, TimeMap[label_to_time_map_index(Param1)], os::chrono::nt_clock::now(), false);
			else
				Dlg->SendMessage(DM_SETFOCUS, Param1 + 1, nullptr);
		}
		break;

	case DN_EDITCHANGE:
		{
			switch (Param1)
			{
			case SA_COMBO_HARDLINK:
			case SA_COMBO_REPARSE_POINT:
				{
					SCOPED_ACTION(Dialog::suppress_redraw)(Dlg);

					FarListInfo li{ sizeof(li) };
					Dlg->SendMessage(DM_LISTINFO, Param1, &li);
					const auto m = Param1 == SA_COMBO_HARDLINK ? lng::MSetAttrHardLinks : lng::MSetAttrDfsTargets;
					Dlg->SendMessage(DM_SETTEXTPTR, Param1, UNSAFE_CSTR(concat(msg(m), L" ("sv, str(li.ItemsNumber), L')')));
				}
				break;

			case SA_EDIT_OWNER:
				DlgParam.Owner.ChangedManually = true;
				break;

			case SA_EDIT_WDATE:
			case SA_EDIT_CDATE:
			case SA_EDIT_ADATE:
			case SA_EDIT_XDATE:
				DlgParam.Times[label_to_time_map_index(Param1 - 1)].Date.ChangedManually = true;
				break;

			case SA_EDIT_WTIME:
			case SA_EDIT_CTIME:
			case SA_EDIT_ATIME:
			case SA_EDIT_XTIME:
				DlgParam.Times[label_to_time_map_index(Param1 - 2)].Time.ChangedManually = true;
				break;
			}
		}
		break;

	case DN_GOTFOCUS:
		{
			if (std::ranges::find(TimeMap, Param1, &time_map::DateId) == std::ranges::cend(TimeMap))
				break;

			if (locale.date_format() != date_type::ymd)
				break;

			if (std::bit_cast<const wchar_t*>(Dlg->SendMessage(DM_GETCONSTTEXTPTR, Param1, nullptr))[0] != L' ')
				break;

			SCOPED_ACTION(Dialog::suppress_redraw)(Dlg);

			COORD Pos;
			Dlg->SendMessage(DM_GETCURSORPOS, Param1, &Pos);

			if (Pos.X != 0)
				break;

			Pos.X = 1;
			Dlg->SendMessage(DM_SETCURSORPOS, Param1, &Pos);
		}
		break;

	default:
		break;
	}

	return Dlg->DefProc(Msg,Param1,Param2);
}

class setarttr_progress: progress_impl
{
	enum
	{
		DlgW = 76,
		DlgH = 5,
	};

	enum items
	{
		pr_doublebox,
		pr_label,
		pr_message,

		pr_count
	};

public:
	setarttr_progress()
	{
		auto ProgressDlgItems = MakeDialogItems<items::pr_count>(
		{
			{ DI_DOUBLEBOX, {{ 3, 1 }, { DlgW - 4, DlgH - 2 }}, DIF_NONE, msg(lng::MSetAttrTitle), },
			{ DI_TEXT,      {{ 5, 2 }, { DlgW - 6,        2 }}, DIF_NONE, msg(lng::MSetAttrSetting) },
			{ DI_TEXT,      {{ 5, 2 }, { DlgW - 6,        2 }}, DIF_SHOWAMPERSAND, {} },
		});

		init(ProgressDlgItems, { -1, -1, DlgW, DlgH });
	}

	void update(string_view const Msg) const
	{
		m_Dialog->SendMessage(DM_SETTEXTPTR, items::pr_message, UNSAFE_CSTR(null_terminated(Msg)));
	}
};

static bool construct_time(
	os::chrono::time_point const OriginalFileTime,
	os::chrono::time_point& FileTime,
	string_view const OSrcDate,
	string_view const OSrcTime)
{
	os::chrono::local_time OriginalLocalTime;
	if (!utc_to_local(OriginalFileTime, OriginalLocalTime))
		return false;

	os::chrono::local_time LocalTime{ parse_time(OSrcDate, OSrcTime, static_cast<int>(locale.date_format())) };

	const auto inherit = [&](auto Getter)
	{
		if (auto& Value = std::invoke(Getter, LocalTime); Value == time_none)
			 Value = std::invoke(Getter, OriginalLocalTime);
	};

	inherit(&os::chrono::local_time::Year);
	inherit(&os::chrono::local_time::Month);
	inherit(&os::chrono::local_time::Day);
	inherit(&os::chrono::local_time::Hours);
	inherit(&os::chrono::local_time::Minutes);
	inherit(&os::chrono::local_time::Seconds);
	inherit(&os::chrono::local_time::Hectonanoseconds);

	return local_to_utc(LocalTime, FileTime);
}

struct state
{
	string const& Owner;
	os::fs::find_data const& FindData;
};

static bool process_single_file(
	const string& Computer,
	string const& Name,
	state const& Current,
	state const& New,
	function_ref<const string&(int)> const DateTimeAccessor,
	bool& SkipErrors)
{
	if (!New.Owner.empty() && !equal_icase(Current.Owner, New.Owner))
	{
		ESetFileOwner(Computer, Name, New.Owner, SkipErrors);
	}

	if (New.FindData.Attributes != Current.FindData.Attributes)
	{
		ESetFileCompression(Name, (New.FindData.Attributes & FILE_ATTRIBUTE_COMPRESSED) != 0, Current.FindData.Attributes, SkipErrors);

		ESetFileEncryption(Name, (New.FindData.Attributes & FILE_ATTRIBUTE_ENCRYPTED) != 0, Current.FindData.Attributes, SkipErrors);

		ESetFileSparse(Name, (New.FindData.Attributes & FILE_ATTRIBUTE_SPARSE_FILE) != 0, Current.FindData.Attributes, SkipErrors);

		const auto IsChanged = [&](os::fs::attributes const Attributes)
		{
			return (New.FindData.Attributes & Attributes) != (Current.FindData.Attributes & Attributes);
		};

		if (IsChanged(FILE_ATTRIBUTE_REPARSE_POINT))
		{
			EDeleteReparsePoint(Name, Current.FindData.Attributes, SkipErrors);
		}

		const auto OtherAttributes = ~(FILE_ATTRIBUTE_ENCRYPTED | FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE | FILE_ATTRIBUTE_REPARSE_POINT);
		if (IsChanged(OtherAttributes))
		{
			ESetFileAttributes(Name, New.FindData.Attributes & OtherAttributes, SkipErrors);
		}
	}

	{
		os::chrono::time_point WriteTime, CreationTime, AccessTime, ChangeTime;
		std::array TimePointers{ &WriteTime, &CreationTime, &AccessTime, &ChangeTime };

		for (const auto& [i, TimePointer] : zip(TimeMap, TimePointers))
		{
			const auto OriginalTime = std::invoke(i.Accessor, Current.FindData);
			if (!construct_time(OriginalTime, *TimePointer, DateTimeAccessor(i.DateId), DateTimeAccessor(i.TimeId))
				|| *TimePointer == OriginalTime)
			{
				TimePointer = {};
			}
		}

		ESetFileTime(Name, TimePointers[0], TimePointers[1], TimePointers[2], TimePointers[3], SkipErrors);
	}

	return true;
}

static auto symlink_type(lng const Id, DWORD const ReparseTag)
{
	if (Id != lng::MSetAttrSymlink)
		return msg(Id);

	const auto Suffix = [ReparseTag]
	{
		switch (ReparseTag)
		{
		case IO_REPARSE_TAG_SYMLINK:
			return L""sv;

		case IO_REPARSE_TAG_DFS:
			return L" DFS"sv;

		case IO_REPARSE_TAG_NFS:
			return L" NFS"sv;

		case IO_REPARSE_TAG_LX_SYMLINK:
			return L" LX"sv;

		default:
			return L" (?)"sv;
		}
	}();

	return far::vformat(msg(Id), Suffix);
}

static bool ShellSetFileAttributesImpl(Panel* SrcPanel, const string* Object)
{
	short DlgX = 80, DlgY = 21;

	const auto C1 = 5;
	const auto C2 = 32;
	const auto TB = 6;
	const auto TBB = TB + 6;

	auto AttrDlg = MakeDialogItems<SA_COUNT>(
	{
		{ DI_DOUBLEBOX, {{ 3,      1     }, {DlgX-4,  DlgY-2}}, DIF_NONE, msg(lng::MSetAttrTitle), },
		{ DI_TEXT,      {{-1,      2     }, {0,       2     }}, DIF_NONE, msg(lng::MSetAttrFor), },
		{ DI_TEXT,      {{5,       3     }, {DlgX-6,  3     }}, DIF_CENTERTEXT | DIF_SHOWAMPERSAND, },
		{ DI_COMBOBOX,  {{5,       3     }, {DlgX-6,  3     }}, DIF_SHOWAMPERSAND | DIF_DROPDOWNLIST | DIF_LISTWRAPMODE | DIF_HIDDEN, },
		{ DI_TEXT,      {{5,       3     }, {17,      3     }}, DIF_HIDDEN, },
		{ DI_EDIT,      {{18,      3     }, {DlgX-6,  3     }}, DIF_HIDDEN | DIF_EDITPATH, },
		{ DI_COMBOBOX,  {{18,      3     }, {DlgX-6,  3     }}, DIF_SHOWAMPERSAND | DIF_DROPDOWNLIST | DIF_LISTWRAPMODE | DIF_HIDDEN, },
		{ DI_TEXT,      {{5,       3     }, {17,      3     }}, DIF_HIDDEN, msg(lng::MSetAttrDiskDevice), },
		{ DI_EDIT,      {{18,      3     }, {DlgX-6,  3     }}, DIF_HIDDEN | DIF_READONLY, },
		{ DI_TEXT,      {{-1,      4     }, {0,       4     }}, DIF_SEPARATOR, },

		{ DI_CHECKBOX,  {{C1,      5     }, {0,       5     }}, DIF_FOCUS, msg(AttributeMap[SA_CHECKBOX_READONLY - SA_ATTR_FIRST].LngId), },
		{ DI_CHECKBOX,  {{C1,      6     }, {0,       6     }}, DIF_NONE, msg(AttributeMap[SA_CHECKBOX_ARCHIVE - SA_ATTR_FIRST].LngId), },
		{ DI_CHECKBOX,  {{C1,      7     }, {0,       7     }}, DIF_NONE, msg(AttributeMap[SA_CHECKBOX_HIDDEN - SA_ATTR_FIRST].LngId), },
		{ DI_CHECKBOX,  {{C1,      8     }, {0,       8     }}, DIF_NONE, msg(AttributeMap[SA_CHECKBOX_SYSTEM - SA_ATTR_FIRST].LngId), },
		{ DI_CHECKBOX,  {{C1,      9     }, {0,       9     }}, DIF_NONE, msg(AttributeMap[SA_CHECKBOX_COMPRESSED - SA_ATTR_FIRST].LngId), },
		{ DI_CHECKBOX,  {{C1,      10    }, {0,       10    }}, DIF_NONE, msg(AttributeMap[SA_CHECKBOX_ENCRYPTED - SA_ATTR_FIRST].LngId), },
		{ DI_CHECKBOX,  {{C1,      11    }, {0,       11    }}, DIF_NONE, msg(AttributeMap[SA_CHECKBOX_NOTINDEXED - SA_ATTR_FIRST].LngId), },
		{ DI_CHECKBOX,  {{C1,      12    }, {0,       12    }}, DIF_NONE, msg(AttributeMap[SA_CHECKBOX_SPARSE - SA_ATTR_FIRST].LngId), },
		{ DI_CHECKBOX,  {{C1,      13    }, {0,       13    }}, DIF_NONE, msg(AttributeMap[SA_CHECKBOX_TEMP - SA_ATTR_FIRST].LngId), },
		{ DI_CHECKBOX,  {{C1,      14    }, {0,       14    }}, DIF_NONE, msg(AttributeMap[SA_CHECKBOX_OFFLINE - SA_ATTR_FIRST].LngId), },
		{ DI_BUTTON,    {{C1,      16    }, {0,       16    }}, DIF_NONE, msg(lng::MSetAttrMore), },

		{ DI_TEXT,      {{C2,      TB+0  }, {0,       TB+0  }}, DIF_NONE, msg(lng::MSetAttrDate), },
		{ DI_TEXT,      {{DlgX-33, TB+0  }, {0,       TB+0  }}, DIF_NONE, },
		{ DI_TEXT,      {{DlgX-21, TB+0  }, {0,       TB+0  }}, DIF_NONE, },
		{ DI_TEXT,      {{C2,      TB+1  }, {0,       TB+1  }}, DIF_NONE, msg(lng::MSetAttrWrite), },
		{ DI_FIXEDIT,   {{DlgX-33, TB+1  }, {DlgX-23, TB+1  }}, DIF_MASKEDIT, },
		{ DI_FIXEDIT,   {{DlgX-21, TB+1  }, {DlgX-6,  TB+1  }}, DIF_MASKEDIT, },
		{ DI_TEXT,      {{C2,      TB+2  }, {0,       TB+2  }}, DIF_NONE, msg(lng::MSetAttrCreation), },
		{ DI_FIXEDIT,   {{DlgX-33, TB+2  }, {DlgX-23, TB+2  }}, DIF_MASKEDIT, },
		{ DI_FIXEDIT,   {{DlgX-21, TB+2  }, {DlgX-6,  TB+2  }}, DIF_MASKEDIT, },
		{ DI_TEXT,      {{C2,      TB+3  }, {0,       TB+3  }}, DIF_NONE, msg(lng::MSetAttrAccess), },
		{ DI_FIXEDIT,   {{DlgX-33, TB+3  }, {DlgX-23, TB+3  }}, DIF_MASKEDIT, },
		{ DI_FIXEDIT,   {{DlgX-21, TB+3  }, {DlgX-6,  TB+3  }}, DIF_MASKEDIT, },
		{ DI_TEXT,      {{C2,      TB+4  }, {0,       TB+4  }}, DIF_NONE, msg(lng::MSetAttrChange), },
		{ DI_FIXEDIT,   {{DlgX-33, TB+4  }, {DlgX-23, TB+4  }}, DIF_MASKEDIT, },
		{ DI_FIXEDIT,   {{DlgX-21, TB+4  }, {DlgX-6,  TB+4  }}, DIF_MASKEDIT, },
		{ DI_BUTTON,    {{0,       TBB   }, {0,       TBB   }}, DIF_CENTERGROUP | DIF_BTNNOCLOSE, msg(lng::MSetAttrOriginal), },
		{ DI_BUTTON,    {{0,       TBB   }, {0,       TBB   }}, DIF_CENTERGROUP | DIF_BTNNOCLOSE, msg(lng::MSetAttrCurrent), },
		{ DI_BUTTON,    {{0,       TBB   }, {0,       TBB   }}, DIF_CENTERGROUP | DIF_BTNNOCLOSE, msg(lng::MSetAttrBlank), },

		{ DI_TEXT,      {{C2,      15    }, {0,       15    }}, DIF_NONE, msg(lng::MSetAttrOwner), },
		{ DI_EDIT,      {{C2,      16    }, {DlgX-6,  16    }}, DIF_NONE, },

		{ DI_TEXT,      {{-1,      17    }, {0,       17    }}, DIF_SEPARATOR | DIF_HIDDEN, },
		{ DI_CHECKBOX,  {{5,       18    }, {0,       18    }}, DIF_DISABLE | DIF_HIDDEN, msg(lng::MSetAttrSubfolders), },

		{ DI_TEXT,      {{-1,      DlgY-4}, {0,       DlgY-4}}, DIF_SEPARATOR, },
		{ DI_BUTTON,    {{0,       DlgY-3}, {0,       DlgY-3}}, DIF_CENTERGROUP | DIF_DEFAULTBUTTON, msg(lng::MSetAttrSet), },
		{ DI_BUTTON,    {{0,       DlgY-3}, {0,       DlgY-3}}, DIF_CENTERGROUP | DIF_DISABLE, msg(lng::MSetAttrSystemDialog), },
		{ DI_BUTTON,    {{0,       DlgY-3}, {0,       DlgY-3}}, DIF_CENTERGROUP, msg(lng::MCancel), },

		{ DI_VTEXT,     {{C2-2,    4     }, {C2-2,    DlgY-4}}, DIF_SEPARATORUSER, },
		{ DI_TEXT,      {{C2-2,    TB+8  }, {DlgX-4,  TB+8  }}, DIF_SEPARATORUSER, },
	});

	AttrDlg[SA_SEPARATOR_VERTICAL].strMask = { BoxSymbols[BS_T_H1V1], BoxSymbols[BS_V1], BoxSymbols[BS_B_H1V1] };
	AttrDlg[SA_SEPARATOR_AFTER_TIME].strMask = { BoxSymbols[BS_L_H1V1], BoxSymbols[BS_H1], BoxSymbols[BS_R_H1V2] };

	// Unofficial margins for DIF_CENTERGROUP
	AttrDlg[SA_BUTTON_ORIGINAL].ListPos = C2;
	AttrDlg[SA_BUTTON_BLANK].ListPos = 6;

	SetAttrDlgParam DlgParam{};
	const size_t SelCount = SrcPanel? SrcPanel->GetSelCount() : 1;

	if (!SelCount)
	{
		return false;
	}

	if(SelCount==1 && os::is_interactive_user_session())
	{
		AttrDlg[SA_BUTTON_SYSTEMDLG].Flags&=~DIF_DISABLE;
	}

	if (SrcPanel && SrcPanel->GetMode() == panel_mode::PLUGIN_PANEL)
	{
		const auto hPlugin = SrcPanel->GetPluginHandle();

		if (hPlugin == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		OpenPanelInfo Info;
		Global->CtrlObject->Plugins->GetOpenPanelInfo(hPlugin,&Info);

		if (!(Info.Flags & OPIF_REALNAMES))
		{
			AttrDlg[SA_BUTTON_SET].Flags|=DIF_DISABLE;
			AttrDlg[SA_BUTTON_SYSTEMDLG].Flags|=DIF_DISABLE;
			DlgParam.Plugin=true;
		}
	}

	FarList NameList{ sizeof(NameList) };
	std::vector<string> Links;
	std::vector<FarListItem> ListItems;

	{
		string SingleSelFileName;
		os::fs::find_data SingleSelFindData;

		if(SrcPanel)
		{
			if (!SrcPanel->get_first_selected(SingleSelFindData))
				return false;

			SingleSelFileName = SingleSelFindData.FileName;
		}
		else
		{
			// BUGBUG
			if (!os::fs::get_find_data(*Object, SingleSelFindData))
			{
				LOGWARNING(L"get_find_data({}): {}"sv, *Object, os::last_error());
			}

			SingleSelFileName = *Object;
		}

		if (SelCount==1 && IsParentDirectory(SingleSelFindData))
			return false;

		const auto DateSeparator = locale.date_separator();
		const auto TimeSeparator = locale.time_separator();
		const auto DecimalSeparator = locale.decimal_separator();

		string DateMask, DateFormat;

		switch (locale.date_format())
		{
		default:
		case date_type::ymd:
			DateMask = far::format(L"N9999{0}99{0}99"sv, DateSeparator);
			DateFormat = far::vformat(msg(lng::MSetAttrDateTitleYMD), DateSeparator);
			break;

		case date_type::dmy:
			DateMask = far::format(L"99{0}99{0}9999N"sv, DateSeparator);
			DateFormat = far::vformat(msg(lng::MSetAttrDateTitleDMY), DateSeparator);
			break;

		case date_type::mdy:
			DateMask = far::format(L"99{0}99{0}9999N"sv, DateSeparator);
			DateFormat = far::vformat(msg(lng::MSetAttrDateTitleMDY), DateSeparator);
			break;
		}

		const auto TimeMask = far::format(L"99{0}99{0}99{1}9999999"sv, TimeSeparator, DecimalSeparator);

		AttrDlg[SA_TEXT_TITLEDATE].strData = DateFormat;
		AttrDlg[SA_TEXT_TITLETIME].strData = far::vformat(msg(lng::MSetAttrTimeTitle), TimeSeparator);

		for (const auto& i: TimeMap)
		{
			AttrDlg[i.DateId].strMask = DateMask;
			AttrDlg[i.TimeId].strMask = TimeMask;
		}

		bool LinkPresent=false;
		string strLinkName;

		const auto EnableSubfolders = [&]
		{
			AttrDlg[SA_SEPARATOR_BEFORE_SUBFOLODERS].Flags &= ~DIF_HIDDEN;
			AttrDlg[SA_CHECKBOX_SUBFOLDERS].Flags &= ~(DIF_DISABLE | DIF_HIDDEN);
			AttrDlg[SA_DOUBLEBOX].Y2 += 2;

			for (const auto i: std::views::iota(SA_SEPARATOR_BEFORE_MAIN_BUTTONS + 0, SA_BUTTON_CANCEL + 1))
			{
				AttrDlg[i].Y1 += 2;
				AttrDlg[i].Y2 += 2;
			}
			DlgY += 2;
		};

		wchar_t DriveLetter{};
		string ComputerName;

		if (SelCount == 1) // !SrcPanel goes here too
		{
			if (!DlgParam.Plugin)
			{
				if (const auto AddFileAttr = os::fs::get_file_attributes(SingleSelFileName); AddFileAttr != INVALID_FILE_ATTRIBUTES)
				{
					if (SingleSelFindData.Attributes == INVALID_FILE_ATTRIBUTES)
						SingleSelFindData.Attributes = 0;

					SingleSelFindData.Attributes |= AddFileAttr;
				}
			}

			if (os::fs::is_directory(SingleSelFindData))
				EnableSubfolders();

			if (SingleSelFindData.Attributes != INVALID_FILE_ATTRIBUTES)
			{
				for (const auto& [i, State]: zip(AttributeMap, DlgParam.Attributes))
				{
					State.InitialValue = (SingleSelFindData.Attributes & i.Attribute) ? BSTATE_CHECKED : BSTATE_UNCHECKED;
				}

				for (const auto& [i, State]: zip(TimeMap, DlgParam.Times))
				{
					std::tie(State.Date.InitialValue, State.Time.InitialValue) = time_point_to_string(std::invoke(i.Accessor, SingleSelFindData));

					AttrDlg[i.DateId].strData = State.Date.InitialValue;
					AttrDlg[i.TimeId].strData = State.Time.InitialValue;
				}
			}

			const auto IsDriveLetterPath = [&]
			{
				if (DlgParam.Plugin)
					return false;

				bool IsRoot = false;
				const auto PathType = ParsePath(SingleSelFileName, nullptr, &IsRoot);
				if (!IsRoot)
					return false;

				switch (PathType)
				{
				case root_type::drive_letter:
					DriveLetter = SingleSelFileName.front();
					return true;

				case root_type::win32nt_drive_letter:
					DriveLetter = SingleSelFileName[L"\\\\?\\"sv.size()];
					return true;

				default:
					return false;
				}
			}();

			const auto IsMountPoint = os::fs::GetVolumeNameForVolumeMountPoint(SingleSelFileName, strLinkName);

			if ((SingleSelFindData.Attributes != INVALID_FILE_ATTRIBUTES && (SingleSelFindData.Attributes & FILE_ATTRIBUTE_REPARSE_POINT)) || IsMountPoint)
			{
				LinkPresent = true;

				auto ID_Msg = IsMountPoint? lng::MSetAttrVolMount : lng::MSetAttrSymlink;
				DWORD ReparseTag = SingleSelFindData.ReparseTag;
				bool KnownReparseTag = false;

				if (!DlgParam.Plugin)
				{
					if (IsMountPoint)
					{
						// BUGBUG, cheating
						ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
						KnownReparseTag = true;
					}
					else
					{
						DWORD ReparseTagAlternative = 0;
						KnownReparseTag = GetReparsePointInfo(SingleSelFileName, strLinkName, &ReparseTagAlternative);
						if (ReparseTagAlternative && !ReparseTag)
						{
							ReparseTag = ReparseTagAlternative;
						}
					}

					if (KnownReparseTag)
					{
						NormalizeSymlinkName(strLinkName);

						switch (ReparseTag)
						{
						case IO_REPARSE_TAG_MOUNT_POINT:

							if (bool Root; IsMountPoint || (ParsePath(strLinkName, nullptr, &Root) == root_type::volume && Root))
								ID_Msg = lng::MSetAttrVolMount;
							else
								ID_Msg = lng::MSetAttrJunction;
							break;

						case IO_REPARSE_TAG_SYMLINK:
							ID_Msg = lng::MSetAttrSymlink;
							break;

						case IO_REPARSE_TAG_APPEXECLINK:
							ID_Msg = lng::MSetAttrAppExecLink;
							break;
						}
					}
					else
					{
							if (ReparseTag == IO_REPARSE_TAG_DFS)
							{
								const auto path = path::join(SrcPanel->GetCurDir(), SingleSelFileName);
								os::netapi::ptr<DFS_INFO_3> DfsInfo;

								auto get_dfs_info = [&](const auto& Callable)
								{
									return Callable(UNSAFE_CSTR(path), {}, {}, 3, edit_as<BYTE**>(&ptr_setter(DfsInfo))) == NERR_Success;
								};

								// Client first - it should be faster and we want to see the activity flag, which is a client thing
								if (get_dfs_info(imports.NetDfsGetClientInfo) || get_dfs_info(imports.NetDfsGetInfo))
								{
									KnownReparseTag = true;

									const std::span DfsStorages(DfsInfo->Storage, DfsInfo->NumberOfStorages);
									ListItems.resize(DfsStorages.size());
									Links.resize(DfsStorages.size());

									for (const auto& [Link, Item, Storage]: zip(Links, ListItems, DfsStorages))
									{
										Link = concat(L"\\\\"sv, Storage.ServerName, path::separator, Storage.ShareName);
										Item.Text = Link.c_str();
										Item.Flags =
											((Storage.State & DFS_STORAGE_STATE_ACTIVE)? (LIF_CHECKED | LIF_SELECTED) : LIF_NONE) |
											((Storage.State & DFS_STORAGE_STATE_OFFLINE)? LIF_GRAYED : LIF_NONE);
									}

									NameList.Items = ListItems.data();
									NameList.ItemsNumber = DfsInfo->NumberOfStorages;
								}
							}

						if (!KnownReparseTag)
						{
							if (reparse_tag_to_string(ReparseTag, strLinkName, Global->Opt->ShowUnknownReparsePoint) || !strLinkName.empty())
								strLinkName = far::format(L"{} {}"sv, replace(msg(lng::MSetAttrReparsePoint), L"&"sv, L""sv), strLinkName);
							else
								strLinkName = msg(lng::MSetAttrUnknownReparsePoint);
						}
					}
				}
				AttrDlg[SA_DOUBLEBOX].Y2++;

				for (auto& i: AttrDlg | std::views::drop(SA_TEXT_REPARSE_POINT))
				{
					++i.Y1;
					++i.Y2;
				}

				AttrDlg[SA_TEXT_REPARSE_POINT].Flags &= ~DIF_HIDDEN;

				if (KnownReparseTag)
				{
					AttrDlg[SA_TEXT_REPARSE_POINT].strData = symlink_type(ID_Msg, ReparseTag);

					if (ReparseTag == IO_REPARSE_TAG_DFS)
					{
						AttrDlg[SA_EDIT_REPARSE_POINT].Flags |= DIF_HIDDEN;
						AttrDlg[SA_COMBO_REPARSE_POINT].Flags &= ~DIF_HIDDEN;
						AttrDlg[SA_COMBO_REPARSE_POINT].ListItems = &NameList;
						AttrDlg[SA_COMBO_REPARSE_POINT].strData = concat(msg(lng::MSetAttrDfsTargets), L" ("sv, str(NameList.ItemsNumber), L')');
					}
					else
					{
						AttrDlg[SA_EDIT_REPARSE_POINT].Flags &= ~DIF_HIDDEN;
						AttrDlg[SA_EDIT_REPARSE_POINT].strData = strLinkName;
					}

					if (IsMountPoint || none_of(ReparseTag, IO_REPARSE_TAG_MOUNT_POINT, IO_REPARSE_TAG_SYMLINK, IO_REPARSE_TAG_APPEXECLINK))
						AttrDlg[SA_EDIT_REPARSE_POINT].Flags |= DIF_READONLY;
				}
				else
				{
					AttrDlg[SA_TEXT_REPARSE_POINT].X2 = AttrDlg[SA_EDIT_REPARSE_POINT].X2;
					AttrDlg[SA_TEXT_REPARSE_POINT].Flags |= DIF_CENTERTEXT;
					AttrDlg[SA_TEXT_REPARSE_POINT].strData = strLinkName;
				}
			}

			// обработка случая "несколько хардлинков"
			if (os::fs::is_file(SingleSelFindData))
			{
				if (const auto Hardlinks = GetNumberOfLinks(SingleSelFileName); Hardlinks && *Hardlinks > 1)
				{
					AttrDlg[SA_TEXT_NAME].Flags|=DIF_HIDDEN;
					AttrDlg[SA_COMBO_HARDLINK].Flags&=~DIF_HIDDEN;

					auto strRoot = GetPathRoot(SingleSelFileName);
					DeleteEndSlash(strRoot);

					NameList.ItemsNumber = *Hardlinks;
					Links.reserve(NameList.ItemsNumber);
					for (const auto& i: os::fs::enum_names(SingleSelFileName))
					{
						Links.emplace_back(strRoot + i);
					}

					if (!Links.empty())
					{
						ListItems.reserve(Links.size());
						std::ranges::transform(Links, std::back_inserter(ListItems), [&](const string& i) { return FarListItem{ 0, i.c_str() }; });

						NameList.Items = ListItems.data();
						AttrDlg[SA_COMBO_HARDLINK].ListItems = &NameList;
					}
					else
					{
						AttrDlg[SA_COMBO_HARDLINK].Flags|=DIF_DISABLE;
					}

					AttrDlg[SA_COMBO_HARDLINK].strData = concat(msg(lng::MSetAttrHardLinks), L" ("sv, str(NameList.ItemsNumber), L')');
				}
			}

			AttrDlg[SA_TEXT_NAME].strData = IsDriveLetterPath?
				os::fs::drive::get_root_directory(DriveLetter) :
				QuoteOuterSpace(SingleSelFileName);

			if (!SrcPanel) // Called from Disk Menu
			{
				if (string Device; os::fs::resolve_kernel_link(DeleteEndSlash(static_cast<string_view>(SingleSelFileName)), Device))
				{
					++DlgY;
					++AttrDlg[SA_DOUBLEBOX].Y2;

					for (auto& i: AttrDlg | std::views::drop(SA_TEXT_DEVICE))
					{
						++i.Y1;
						++i.Y2;
					}

					AttrDlg[SA_TEXT_DEVICE].Flags &= ~DIF_HIDDEN;
					AttrDlg[SA_EDIT_DEVICE].Flags &= ~DIF_HIDDEN;
					AttrDlg[SA_EDIT_DEVICE].strData = Device;
				}
			}

			ComputerName = ExtractComputerName(SrcPanel?
				SrcPanel->GetCurDir() :
				ConvertNameToFull(SingleSelFileName));

			GetFileOwner(ComputerName, SingleSelFileName, DlgParam.Owner.InitialValue);
		}
		else
		{
			for (auto& i: DlgParam.Attributes)
			{
				i.Flags |= DIF_3STATE;
				i.InitialValue = BSTATE_UNCHECKED;
			}

			AttrDlg[SA_BUTTON_ORIGINAL].Flags|=DIF_DISABLE;
			AttrDlg[SA_TEXT_NAME].strData = msg(lng::MSetAttrSelectedObjects);

			// проверка - есть ли среди выделенных - каталоги?
			// так же проверка на атрибуты
			auto FolderPresent = false;

			ComputerName = ExtractComputerName(SrcPanel->GetCurDir());

			bool CheckOwner=true;

			std::optional<os::chrono::time_point> Times[std::size(TimeMap)];
			bool SkipCheckTimes[std::size(TimeMap)]{};

			for (const auto& PanelItem: SrcPanel->enum_selected())
			{
				if (!FolderPresent && os::fs::is_directory(PanelItem))
				{
					FolderPresent = true;
					EnableSubfolders();
				}

				for (const auto& [Attr, State] : zip(AttributeMap, DlgParam.Attributes))
				{
					if (PanelItem.Attributes & Attr.Attribute)
					{
						++State.InitialValue;
					}
				}

				if(CheckOwner)
				{
					string strCurOwner;
					GetFileOwner(ComputerName, PanelItem.FileName, strCurOwner);
					if(DlgParam.Owner.InitialValue.empty())
					{
						DlgParam.Owner.InitialValue = strCurOwner;
					}
					else if(DlgParam.Owner.InitialValue != strCurOwner)
					{
						DlgParam.Owner.InitialValue.clear();
						CheckOwner=false;
					}
				}

				for (const auto& [t, State, DestTime, SkipCheckTime]: zip(TimeMap, DlgParam.Times, Times, SkipCheckTimes))
				{
					if (SkipCheckTime)
						continue;

					const auto SrcTime = std::invoke(t.Accessor, PanelItem);
					if (!DestTime)
					{
						DestTime = SrcTime;
					}
					else if (*DestTime != SrcTime)
					{
						DestTime.reset();
						SkipCheckTime = true;
					}
				}
			}

			SrcPanel->get_first_selected(SingleSelFindData);
			SingleSelFileName = SingleSelFindData.FileName;

			for (auto& i: DlgParam.Attributes)
			{
				if (!FolderPresent && (!i.InitialValue || static_cast<size_t>(i.InitialValue) == SelCount))
					i.Flags &= ~DIF_3STATE;

				i.InitialValue = static_cast<size_t>(i.InitialValue) == SelCount?
					BST_CHECKED :
					i.InitialValue?
						BSTATE_3STATE :
						BSTATE_UNCHECKED;
			}

			for (const auto& [i, State, Time]: zip(TimeMap, DlgParam.Times, Times))
			{
				if (!Time)
					continue;

				std::tie(State.Date.InitialValue, State.Time.InitialValue) = time_point_to_string(*Time);

				AttrDlg[i.DateId].strData = State.Date.InitialValue;
				AttrDlg[i.TimeId].strData = State.Time.InitialValue;
			}
		}

		for (auto& i: DlgParam.Attributes)
		{
			i.CurrentValue = i.InitialValue;
		}

		if (!Global->Opt->SetAttrFolderRules)
		{
			AttrDlg[SA_CHECKBOX_SUBFOLDERS].Selected = BSTATE_CHECKED;

			for (auto& i: DlgParam.Attributes)
			{
				i.Flags |= DIF_3STATE;
				i.CurrentValue = BSTATE_3STATE;
			}
		}

		for (const auto i: std::views::iota(SA_ATTR_FIRST + 0, SA_ATTR_LAST + 1))
		{
			auto& DlgItem = AttrDlg[i];
			const auto& State = DlgParam.Attributes[i - SA_ATTR_FIRST];

			DlgItem.Flags |= State.Flags;
			DlgItem.Selected = State.CurrentValue;
		}

		AttrDlg[SA_EDIT_OWNER].strData = DlgParam.Owner.InitialValue;

		DlgParam.strSelName = SingleSelFileName;
		DlgParam.DialogMode = SelCount == 1?
			(SingleSelFindData.Attributes & FILE_ATTRIBUTE_DIRECTORY)? MODE_FOLDER : MODE_FILE :
			MODE_MULTIPLE;


		const auto Dlg = Dialog::create(AttrDlg, SetAttrDlgProc, &DlgParam);
		Dlg->SetHelp(L"FileAttrDlg"sv);
		Dlg->SetId(FileAttrDlgId);

		if (LinkPresent)
		{
			DlgY++;
		}

		Dlg->SetPosition({ -1, -1, DlgX, DlgY });
		Dlg->Process();

		switch(Dlg->GetExitCode())
		{
		case SA_BUTTON_SET:
			{
				//reparse point editor, can be used even if the attributes are invalid (e.g. a CD drive)
				if (!equal_icase(AttrDlg[SA_EDIT_REPARSE_POINT].strData, strLinkName))
				{
					bool Dummy = false;
					retryable_ui_operation([&]{ return ModifyReparsePoint(SingleSelFileName, os::env::expand(unquote(AttrDlg[SA_EDIT_REPARSE_POINT].strData))); }, SingleSelFileName, lng::MCopyCannotCreateLink, Dummy);
				}

				if (SingleSelFindData.Attributes == INVALID_FILE_ATTRIBUTES)
					return true;

				os::fs::attributes SetAttr = 0, ClearAttr = 0;

				for (const auto& [i, Attr]: zip(DlgParam.Attributes, AttributeMap))
				{
					switch (i.CurrentValue)
					{
					case BSTATE_CHECKED:
						SetAttr |= Attr.Attribute;
						break;

					case BSTATE_UNCHECKED:
						ClearAttr |= Attr.Attribute;
						break;
					}
				}

				for (const auto& i: TimeMap)
				{
					AttrDlg[i.TimeId].strData[8] = locale.time_separator();
				}

				setarttr_progress const Progress;

				if (SelCount == 1)
				{
					Progress.update(SingleSelFileName);

					Global->WindowManager->PluginCommit();
					Global->ScrBuf->Flush();
				}

				auto SkipErrors = false;

				const auto AttrDlgAccessor = [&AttrDlg](int const Id) -> auto& { return AttrDlg[Id].strData; };

				if (SelCount==1 && !(SingleSelFindData.Attributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					os::fs::find_data NewFindData;
					NewFindData.Attributes = (SingleSelFindData.Attributes | SetAttr) & ~ClearAttr;

					const state
						Current{ DlgParam.Owner.InitialValue, SingleSelFindData },
						New{ AttrDlg[SA_EDIT_OWNER].strData, NewFindData };

					if (!process_single_file(ComputerName, SingleSelFileName, Current, New, AttrDlgAccessor, SkipErrors))
					{
						return false;
					}
				}
				else
				{
					ConsoleTitle::SetFarTitle(msg(lng::MSetAttrTitle));

					if(SrcPanel)
					{
						Global->CtrlObject->Cp()->GetAnotherPanel(SrcPanel)->CloseFile();
						SrcPanel->GetSelName(nullptr);
					}

					SCOPED_ACTION(taskbar::indeterminate);
					SCOPED_ACTION(wakeful);

					const time_check TimeCheck(time_check::mode::immediate);

					while (!SrcPanel || SrcPanel->GetSelName(&SingleSelFileName, nullptr, &SingleSelFindData))
					{
						if (TimeCheck)
						{
							if (CheckForEscAndConfirmAbort())
								break;

							Progress.update(SingleSelFileName);
						}

						{
							os::fs::find_data NewFindData;
							NewFindData.Attributes = (SingleSelFindData.Attributes | SetAttr) & ~ClearAttr;

							string Empty;

							const state
								Current{ AttrDlg[SA_CHECKBOX_SUBFOLDERS].Selected? Empty : DlgParam.Owner.InitialValue, SingleSelFindData},
								New{ AttrDlg[SA_EDIT_OWNER].strData, NewFindData };

							if (!process_single_file(ComputerName, SingleSelFileName, Current, New, AttrDlgAccessor, SkipErrors))
							{
								return false;
							}
						}

						if ((SingleSelFindData.Attributes & FILE_ATTRIBUTE_DIRECTORY) && AttrDlg[SA_CHECKBOX_SUBFOLDERS].Selected)
						{
							ScanTree ScTree(false);
							ScTree.SetFindPath(SingleSelFileName, L"*"sv);
							const time_check TreeTimeCheck;
							string strFullName;

							while (ScTree.GetNextName(SingleSelFindData, strFullName))
							{
								if (TreeTimeCheck)
								{
									Progress.update(strFullName);

									if (CheckForEscAndConfirmAbort())
									{
										return false;
									}
								}

								os::fs::find_data NewFindData;
								NewFindData.Attributes = (SingleSelFindData.Attributes | SetAttr) & ~ClearAttr;

								string Empty;

								const state
									Current{ Empty, SingleSelFindData }, // BUGBUG, should we read the owner?
									New{ AttrDlg[SA_EDIT_OWNER].strData, NewFindData };

								if (!process_single_file(ComputerName, strFullName, Current, New, AttrDlgAccessor, SkipErrors))
								{
									return false;
								}
							}
						}

						if (!SrcPanel)
							break;
					}
				}
			}
			break;
		case SA_BUTTON_SYSTEMDLG:
			{
				if (!os::is_interactive_user_session())
					return true;

				SHELLEXECUTEINFO seInfo{ sizeof(seInfo) };
				seInfo.nShow = SW_SHOW;
				seInfo.fMask = SEE_MASK_INVOKEIDLIST;

				string strFullName;

				if (DriveLetter)
				{
					// \\?\X:\ doesn't work on old Windows versions
					// X: works everywhere
					strFullName = os::fs::drive::get_device_path(DriveLetter);
				}
				else
				{
					strFullName = SingleSelFileName;

					if (SingleSelFindData.Attributes & FILE_ATTRIBUTE_DIRECTORY)
						AddEndSlash(strFullName);
				}

				seInfo.lpFile = strFullName.c_str();
				seInfo.lpVerb = L"properties";
				const auto strCurDir = os::fs::get_current_directory();
				seInfo.lpDirectory=strCurDir.c_str();
				ShellExecuteEx(&seInfo);
			}
			return false;

		default:
			return false;
		}
	}

	if(SrcPanel)
	{
		SrcPanel->SaveSelection();
		SrcPanel->Update(UPDATE_KEEP_SELECTION);
		SrcPanel->ClearSelection();
		Global->CtrlObject->Cp()->GetAnotherPanel(SrcPanel)->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
	}
	Global->WindowManager->RefreshWindow(Global->CtrlObject->Panels());
	return true;
}

void ShellSetFileAttributes(Panel* SrcPanel, const string* Object)
{
	try
	{
		ShellSetFileAttributesImpl(SrcPanel, Object);
	}
	catch (operation_cancelled const&)
	{
		// Nop
	}
}
