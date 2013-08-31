function exec(command)
   return function () os.execute(command) end
end

function get_shortcut_from_text(text)
   idx = text:find('&')

   if idx == #text then
      return nil
   else
      return text:sub(idx+1,idx+1)
   end
end

function strip_shortcut_from_text(text)
   return text:gsub('&', '', 1)
end

function table_to_items(tbl)
   for n, val in ipairs(tbl) do
      if val.__index == undefined then

         kw = {
            text = val[1],
            action = val[2],
            icon = val[3],
            shortcut = get_shortcut_from_text(val[1])
         }
         it = item()
      end

   end
   
end
