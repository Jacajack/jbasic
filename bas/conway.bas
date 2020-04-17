# Dimensions
w = 20
h = 12

# Array declarations
IDIM map (w*h);
IDIM nc (w*h);

# Load from stdin
y = 0
while y < h
	x = 0
	while x < w
		c = getchar()
		if c != 10 and c != 13
			map(y*w + x) = c
			# putchar(c)
			x = x + 1
		end
	end
	y = y + 1
end


ticks = 100 + 1

# The main loop
n = 0
change_count = 1
while n < ticks && change_count
	println "----------------------------------"
	print "After "
	print n
	println " iterations"

	# Show the map and count neighbors
	y = 0
	while y < h
		x = 0
		while x < w
			c = map(y*w+x)

			# Neighbor count
			cnt = 0
			cnt = cnt + (map(((y-1) mod h) * w + (x-1) mod w) == 88);
			cnt = cnt + (map(((y-1) mod h) * w + (x+0) mod w) == 88);
			cnt = cnt + (map(((y-1) mod h) * w + (x+1) mod w) == 88);
			cnt = cnt + (map(((y+0) mod h) * w + (x-1) mod w) == 88);
			cnt = cnt + (map(((y+0) mod h) * w + (x+1) mod w) == 88);
			cnt = cnt + (map(((y+1) mod h) * w + (x-1) mod w) == 88);
			cnt = cnt + (map(((y+1) mod h) * w + (x+0) mod w) == 88);
			cnt = cnt + (map(((y+1) mod h) * w + (x+1) mod w) == 88);

			nc(y*w+x) = cnt
			putchar(c)
			x = x + 1
		end
		putchar(10)
		y = y + 1
	end

	# Show neighbors and do magic
	y = 0
	change_count = 0
	while y < h
		x = 0
		while x < w

			#print(1*nc(y*w+x))

			# Birth
			if map(y*w+x) != 88 && nc(y*w+x) == 3
				map(y*w+x) = 88
				change_count = change_count + 1
			else
				# Death
				if map(y*w+x) == 88 && nc(y*w+x) != 2 && nc(y*w+x) != 3
					map(y*w+x) = 46
					change_count = change_count + 1
				end
			end

			x = x + 1
		end
		#putchar(10)
		y = y + 1
	end

	n = n + 1
end
