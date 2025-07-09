local t = Def.ActorFrame {};

local t_sleep = 4.2;
local piu_grades = { "SSS+", "SSS", "SS+", "SS", "S+", "S", "AAA+", "AAA", "AA+", "AA", "A+", "A", "B", "C", "D", "F" };
local piu_grades_old = { "SSS", "SSS", "X", "X", "G", "G", "A", "A", "A", "A", "A", "A", "B", "C", "D", "F" };
local new_record_delays = { 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 2, 1, 1, 1};

local p1_joined = GAMESTATE:IsSideJoined(PLAYER_1);
local p2_joined = GAMESTATE:IsSideJoined(PLAYER_2);

local p1_grade = 21; -- Grade_Fail
local p2_grade = 21; -- Grade_Fail

local stage_break = { PLAYER_1 = false, PLAYER_2 = false };

if p1_joined then
	local p1_pss = STATSMAN:GetCurStageStats():GetPlayerStageStats(PLAYER_1)
	local p1_score = (math.floor(p1_pss:GetScore()/100)) - 1000000;
	if p1_pss:GetReachedLifeZero() then
		stage_break[PLAYER_1] = true;
		p1_score = p1_score + 1000000;
	end
	p1_grade = CalcPGradeInt(p1_score);
end;

if p2_joined then
	local p2_pss = STATSMAN:GetCurStageStats():GetPlayerStageStats(PLAYER_2)
	local p2_score = (math.floor(p2_pss:GetScore()/100)) - 1000000;
	if p2_pss:GetReachedLifeZero() then
		stage_break[PLAYER_2] = true;
		p2_score = p2_score + 1000000;
	end
	p2_grade = CalcPGradeInt(p2_score);
end;

if p1_grade == 21 then
	p1_grade = 16;
end;

if p2_grade == 21 then
	p2_grade = 16;
end;

local function GradeActor(pn)
	local igrade;
	if pn == PLAYER_1 then igrade = p1_grade; end;
	if pn == PLAYER_2 then igrade = p2_grade; end;
	
	-- 21 corresponde a Grade_Fail
	if igrade == 21 then
		igrade = 16;
	end;
	
	local grade = piu_grades[igrade];
	
	local condition = stage_break[pn] and "B" or "R"
	
	local t = Def.ActorFrame {};
	t[#t+1] = LoadActor( THEME:GetPathG("","ScreenEvaluation/"..condition.."_"..grade..".png") )..{
		InitCommand=cmd(basezoom,.66);
		OnCommand=cmd(zoom,1.5;diffusealpha,0;sleep,t_sleep;linear,.2;zoom,1;diffusealpha,1);
	};

	t[#t+1] = LoadActor( THEME:GetPathG("","ScreenEvaluation/"..condition.."_"..grade..".png") )..{
		InitCommand=cmd(basezoom,.66;blend,'BlendMode_Add');
		OnCommand=cmd(zoom,.5;diffusealpha,0;sleep,t_sleep;sleep,.2;diffusealpha,1;decelerate,.35;zoom,2;diffusealpha,0);
	};
	--[[
	t[#t+1] = LoadActor( THEME:GetPathG("","ScreenEvaluation/R_"..grade.."_SB.png") )..{
		InitCommand=cmd(basezoom,.66);
		OnCommand=cmd(zoom,1;diffusealpha,0;sleep,t_sleep;sleep,.2;sleep,.35;sleep,1;diffusealpha,1);
	};

	t[#t+1] = LoadActor( THEME:GetPathG("","ScreenEvaluation/GLASS.png") )..{
		InitCommand=cmd(basezoom,.66;blend,'BlendMode_Add');
		OnCommand=cmd(zoom,.8;diffusealpha,0;sleep,t_sleep;sleep,.2;sleep,.35;sleep,1;diffusealpha,1;decelerate,1;zoom,1.2;diffusealpha,0);
	};]]--
	if igrade==1 then
	t[#t+1] = LoadActor( THEME:GetPathG("","ScreenEvaluation/_shine.png") )..{
		InitCommand=cmd(basezoom,.66;blend,'BlendMode_Add');
		OnCommand=cmd(zoom,3.5;diffusealpha,0;sleep,t_sleep;diffusealpha,1;linear,.2;zoom,1;diffusealpha,1;decelerate,.4;zoom,5;diffusealpha,0);
	};
	end;
	return t;
end;

-- Grades
if p1_joined then
	t[#t+1] = GradeActor(PLAYER_1)..{
		InitCommand=cmd(x,SCREEN_CENTER_X-156;y,SCREEN_CENTER_Y-40);
	};
end;

if p2_joined then
	t[#t+1] = GradeActor(PLAYER_2)..{
		InitCommand=cmd(x,SCREEN_CENTER_X+156;y,SCREEN_CENTER_Y-40);
	};
end;


-- Sounds
local BestGrade;

if p1_joined and p2_joined then
	if p1_grade > p2_grade then
		BestGrade = p2_grade;
	else
		BestGrade = p1_grade;
	end;
end;

if p1_joined and not p2_joined then
	BestGrade = p1_grade;
end;

if not p1_joined and p2_joined then
	BestGrade = p2_grade;
end;

local grade_sound = piu_grades[BestGrade]
t[#t+1] = Def.Sound {
	InitCommand=function(self)
		self:load(THEME:GetPathS('','Rank/RANK_'..piu_grades_old[BestGrade]..'.mp3'));
	end;
	OnCommand=cmd(sleep,t_sleep+.2;queuecommand,'Play');
	PlayCommand=cmd(play);
	GoNextScreenMessageCommand=cmd(pause);
};

t[#t+1] = Def.Sound {
	InitCommand=function(self)
		self:load(THEME:GetPathS('','Rank/RANK_'..piu_grades_old[BestGrade]..'_B.mp3'));
	end;
	OnCommand=cmd(sleep,t_sleep+.2;queuecommand,'Play');
	PlayCommand=cmd(play);
	GoNextScreenMessageCommand=cmd(pause);
};

t[#t+1] = Def.Sound {
	InitCommand=function(self)
		self:load(THEME:GetPathS('','Rank/RANK.mp3'));
	end;
	OnCommand=cmd(sleep,t_sleep+.2;queuecommand,'Play');
	PlayCommand=cmd(play);
	GoNextScreenMessageCommand=cmd(pause);
};
--[[
t[#t+1] = LoadActor(THEME:GetPathS("","Rank/GLASS.mp3"))..{
	OnCommand=cmd(sleep,t_sleep;sleep,.2;sleep,.35;sleep,1;queuecommand,'Play');
	PlayCommand=function(self)
		self:play();
	end;
	--OffCommand=cmd(stoptweening;pause);
	GoNextScreenMessageCommand=cmd(pause);
}]]--

t[#t+1] = LoadActor(THEME:GetPathS("","Sounds/NEW_RECORD.mp3"))..{
	OnCommand=function(self)
		if SCREENMAN:GetTopScreen():PlayerHasNewRecord(PLAYER_1) or SCREENMAN:GetTopScreen():PlayerHasNewRecord(PLAYER_2) or 
		SCREENMAN:GetTopScreen():PlayerHasNewMachineRecord(PLAYER_1) or SCREENMAN:GetTopScreen():PlayerHasNewMachineRecord(PLAYER_2)
		then
			(cmd(sleep,t_sleep+.2+  new_record_delays[BestGrade] + .5;queuecommand,'Play'))(self);
		end;
	end;
	PlayCommand=function(self)
		self:play();
	end;
	--OffCommand=cmd(stoptweening;pause);
	GoNextScreenMessageCommand=cmd(pause);
}

return t;
