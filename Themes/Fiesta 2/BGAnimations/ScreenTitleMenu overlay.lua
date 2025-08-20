local t = Def.ActorFrame {};

t[#t+1] = LoadFont("Common Bold")..{
	InitCommand=function(self)
		self:x(SCREEN_CENTER_X);
		self:y(SCREEN_TOP + 40);
		self:settext("Stepmania 5.1\nStepP1 Legacy by xMAx; Pumpmania (StepP1Revival) by Silver and Thequila\nPhyrebird Theme by Team Phyrebird and Drako")
		self:skewx(-.12);
		self:zoom(.35);
	end;
	OffCommand=function(self)
		SCREENMAN:GetTopScreen():lockinput(.5);	-- para que cualquier entrada no interrumpa la transici�n de ventanas
		(cmd(visible,false))(self);
	end;
}

return t;