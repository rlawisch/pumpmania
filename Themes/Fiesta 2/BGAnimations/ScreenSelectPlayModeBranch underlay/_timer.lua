local t = Def.ActorFrame {}



t[#t+1] = LoadActor(THEME:GetPathG("","Common Resources/TIMER_MASK.png") )..{
	OnCommand=function(self)
		self:zoom(0.075)
		self:x(cx)
		self:y(22)
		self:play()
		self:MaskSource()
	end
}

t[#t+1] = Def.Sprite {
    Texture = THEME:GetPathG("", "Common Resources/TIMER_FRAME 5x8.png"),
    OnCommand = function(self)
        self:SetAllStateDelays(0.02)
        self:setstate(0)
        self:animate(true)

        self:zoom(0.325)
        self:x(cx)
        self:y(22)
        self:MaskDest()
    end
}

return t;