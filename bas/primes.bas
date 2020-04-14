top = 40

n = 2
while (n - top)
	ok = 1
	
	i = 2
	while i - n
		ok = ok and (n % i)
		i = i + 1
	end

	if ok
		print n+0;
	end

	n = n + 1
end
