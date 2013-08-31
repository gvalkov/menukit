-- menu entry

local item = {}
item.__index = item

local item_mt = {
   __call = function(cls, ...) return cls.new(...) end,
}

setmetatable(item, item_mt)

function item.new(text, action, icon, shortcut, style)
   local self = setmetatable({}, item)

   self.text = text
   self.action = action
   self.icon = icon
   self.shortcut = shortcut
   self.style = style

   return self
end


-- menu 

local menu = {}
menu.__index = menu

local menu_mt = {
   __call = function(cls, ...) return cls.new(...) end,
}

setmetatable(menu, menu_mt)

function menu.new(items)
   local self = setmetatable({}, menu)
   self.items = items
   return self
end
