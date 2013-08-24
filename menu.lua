function list_firefox_profiles(ini)
   if ini == nil then
      ini = os.getenv('HOME')..'/.mozilla/firefox/profiles.ini'
   end

   local res = {}
   if awful.util.file_readable(ini) then
      for line in io.lines(ini) do
         local _, _, name = line:find('^Name=(.*)')

         if name ~= nil then
            table.insert(res, name)
         end
      end
   end
   
   return res
end

function firefox_profiles_menu(ini)
   local profiles = list_firefox_profiles(ini)
   local menu = {}

   for n, name in ipairs(profiles) do
      local item = {name, 'firefox -P '..name, util.icon('firefox')}
      table.insert(menu, item)
   end

   return menu
end

browsers = {
   { '&firefox', exec('firefox')  },
   { '&chrome', exec('google-chrome') },
   { 'ff &profiles', menu(firefox_profiles_menu) }
}


--menus.freedesktop = util.load_or_create_freedesktop_menu()
menus.awesome = {
   { 'manual', terminal .. ' -e man awesome' },
   { 'edit config', editor_cmd .. ' ' .. awesome.conffile },
   { '&restart', awesome.restart },
   { 'quit', awesome.quit }
}

menus.browsers = {
   { '&firefox', 'firefox', util.icon('firefox') },
   { 'firefox &private', 'firefox -new-instance -private', util.icon('firefox') },
   { 'ff &profile', util.firefox_profiles_menu(), util.icon('firefox') },
   { '&chrome', 'google-chrome', util.icon('google-chrome') },
}

menus.main = awful.menu({
   items = { { '&browsers', menus.browsers },
             { '&xdg', menus.freedesktop },
             { '&awesome', menus.awesome, beautiful.awesome_icon },
             { '&session', sessionmenu.menu() },
             { 'open terminal', terminal }
   }
})
