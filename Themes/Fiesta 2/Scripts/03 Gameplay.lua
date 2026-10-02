-- sm-ssc fallback theme | script ring 03 | Gameplay.lua
-- [en] This file is used to store settings that should be different in each
-- game mode.

-- shakesoda calls this pump.lua
local function CurGameName()
	return GAMESTATE:GetCurrentGame():GetName()
end

-- Check the active game mode against a string. Cut down typing this in metrics.
function IsGame(str) return CurGameName():lower() == str:lower() end

-- GetExtraColorThreshold()
-- [en] returns the difficulty threshold in meter
-- for songs that should be counted as boss songs.
function GetExtraColorThreshold()
	local Modes = {
		pump = 21,
		lights = 10, -- lights shouldn't be playable
	}
	return Modes[CurGameName()] or 10
end

-- AllowOptionsMenu()
-- [en] returns if you are able to select options
-- on ScreenSelectMusic.
function AllowOptionsMenu()
  if GAMESTATE:IsAnExtraStage() then
    return false
  elseif GAMESTATE:GetPlayMode() == "PlayMode_Oni" then 
    return false
  else
    return true
  end
end

-- GameCompatibleModes:
-- [en] returns possible modes for ScreenSelectPlayMode
function GameCompatibleModes()
	local Modes = {
		pump = "Single,Double,HalfDouble,Versus,Couple,Routine",
		lights = "Single" -- lights shouldn't be playable
	}
	return Modes[CurGameName()]
end

function SelectProfileKeys()
	local sGame = CurGameName()
	if sGame == "pump" then
		return "Up,Down,Start,Back,Center,DownLeft,DownRight"
	else
		return "Up,Down,Start,Back"
	end
end

-- ScoreKeeperClass:
-- [en] Determines the correct ScoreKeeper class to use.
function ScoreKeeperClass()
	-- rave scorekeeper
	if GAMESTATE:GetPlayMode() == 'PlayMode_Rave' then return "ScoreKeeperRave" end
	if GAMESTATE:GetCurrentStyle() then
		local styleType = GAMESTATE:GetCurrentStyle():GetStyleType()
		if styleType == 'StyleType_TwoPlayersSharedSides' then return "ScoreKeeperShared" end
	end
	return "ScoreKeeperNormal"
end

-- ComboContinue:
-- [en] 
function ComboContinue()
	local Continue = {
		pump = "TapNoteScore_W3",
	}
  return Continue[CurGameName()] or "TapNoteScore_W3"
end

function ComboMaintain()
	local Maintain = {
		pump = "TapNoteScore_W4",
	}
  return Maintain[CurGameName()] or "TapNoteScore_W3"
end

function ComboPerRow()
	sGame = CurGameName()
	if sGame == "pump" then
		return true
	elseif GAMESTATE:GetPlayMode() == "PlayMode_Oni" then
		return true
	else
		return false
	end
end

function EvalUsesCheckpointsWithJudgments()
	return (CurGameName() == "pump") and true or false
end

local ComboThresholds = {
	pump	= { Hit = 4, Miss = 4, Fail = 51 },
	-------------------------------------------
	default	= { Hit = 2, Miss = 2, Fail = -1 }
}

function HitCombo()
	if ComboThresholds[CurGameName()] then
		return ComboThresholds[CurGameName()].Hit
	end
	return ComboThresholds["default"].Hit
end

function MissCombo()
	if ComboThresholds[CurGameName()] then
		return ComboThresholds[CurGameName()].Miss
	end
	return ComboThresholds["default"].Miss
end

-- FailCombo:
-- [en] The combo that causes game failure.
function FailCombo()
	-- ITG (dance) uses 30. Pump Pro uses 30, real Pump uses 51
	if ComboThresholds[CurGameName()] then
		return ComboThresholds[CurGameName()].Fail
	end
	return ComboThresholds["default"].Fail
end

local RoutineSkins = {
	pump	= { P1 = "cmd-routine-p1", P2 = "cmd-routine-p2" },
	-------------------------------------------------------------
	default	= { P1 = "default", P2 = "default" }
}

function RoutineSkinP1()
	if RoutineSkins[CurGameName()] then
		return RoutineSkins[CurGameName()].P1
	end
	return RoutineSkins["default"].P1
end

function RoutineSkinP2()
	if RoutineSkins[CurGameName()] then
		return RoutineSkins[CurGameName()].P2
	end
	return RoutineSkins["default"].P2
end

-- todo: use tables for some of these -aj
function HoldTiming()
	return IsGame("pump") and 0 or PREFSMAN:GetPreference("TimingWindowSecondsHold")
end

function ShowHoldJudgments()
	return not IsGame("pump")
end

AfterGamePlay = function()
	if STATSMAN:GetCurStageStats():AllFailed() then
		return "ScreenStageBreak";
	else
		return "ScreenAfterGameplayDelay";
	end;
end;