local t = Def.ActorFrame {}

t[#t+1] = LoadActor(GetBackgroundPath()) .. {
	InitCommand=cmd(show_background_properly)
}

if GAMESTATE:IsSideJoined(PLAYER_1) then
	t[#t+1] = Def.Sprite {
		Texture=THEME:GetPathG("", "ScreenStageInformation/StepArtist"),
		InitCommand=function(self)
			self:xy(58, SCREEN_BOTTOM-21)
			self:zoom(0.67)
		end
	}
	
	t[#t+1] = Def.BitmapText {
		Font="_myriad pro 20px",
		InitCommand=function(self)
			self:xy(58, SCREEN_BOTTOM-16)
			self:zoom(0.6)
			self:horizalign(center)
			self:wrapwidthpixels(130)
			self:vertspacing(-8)
			self:maxheight(30)
			self:maxwidth(130)
		end,
		OnCommand=function(self)
			if GAMESTATE:GetCurrentSteps(PLAYER_1):GetAuthorCredit() ~= "" then
				self:settext(GAMESTATE:GetCurrentSteps(PLAYER_1):GetAuthorCredit())
			else
				self:settext("Unknown")
			end
		end
	}

	t[#t+1] = GetBallLevelColor( PLAYER_1, false )..{ 
		InitCommand=cmd(basezoom,.57;x,cx-275;playcommand,"ShowUp";y,SCREEN_BOTTOM-40;pause;); 
	};
	t[#t+1] = GetBallLevelTextP1( PLAYER_1, false )..{ 
		InitCommand=cmd(basezoom,.57;x,cx-275;playcommand,"ShowUp";y,SCREEN_BOTTOM-40;pause;); 
	};

end

if GAMESTATE:IsSideJoined(PLAYER_2) then
	t[#t+1] = Def.Sprite {
		Texture=THEME:GetPathG("", "ScreenStageInformation/StepArtist"),
		InitCommand=function(self)
			self:xy(SCREEN_RIGHT-58, SCREEN_BOTTOM-21)
			self:zoom(0.67)
		end
	}
	
	t[#t+1] = Def.BitmapText {
		Font="_myriad pro 20px",
		InitCommand=function(self)
			self:xy(SCREEN_RIGHT-58, SCREEN_BOTTOM-16)
			self:zoom(0.6)
			self:horizalign(center)
			self:wrapwidthpixels(130)
			self:vertspacing(-8)
			self:maxheight(30)
			self:maxwidth(130)
		end,
		OnCommand=function(self)
			if GAMESTATE:GetCurrentSteps(PLAYER_2):GetAuthorCredit() ~= "" then
				self:settext(GAMESTATE:GetCurrentSteps(PLAYER_2):GetAuthorCredit())
			else
				self:settext("Unknown")
			end
		end
	}
	
	t[#t+1] = GetBallLevelColor( PLAYER_2, false )..{ 
	InitCommand=cmd(basezoom,.57;zoomx,-1;x,cx+275;playcommand,"ShowUp";y,SCREEN_BOTTOM-40;pause;); 
	};
	t[#t+1] = GetBallLevelTextP2( PLAYER_2, false )..{ 
		InitCommand=cmd(basezoom,.57;x,cx+275;playcommand,"ShowUp";y,SCREEN_BOTTOM-40;pause;); 
	};
end

return t


