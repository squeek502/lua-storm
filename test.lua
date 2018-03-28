local storm = require('storm')

local mpq = storm.open('Patch_D2.mpq')
print(mpq)

local filename = 'data\\global\\excel\\MonLvl.txt'
local contents = assert(mpq:read(filename))
print(#contents)

local files = assert(mpq:files())
for _, file in ipairs(files) do
	print(file.name, file.size, file.basename)
end

assert(mpq:extract('data\\global\\excel\\MonStats.txt', 'MonStats.txt'))