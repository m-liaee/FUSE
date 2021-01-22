path = "/tmp/fs-part4.log"
with open(path,'r') as f: 
	f.readline(); 
	f.readline(); 	
	opr1 = '' 
	while  opr1 != 'read' :  
		first_time = 0
		line1 = f.readline() 
		if line1 == "": break
		words = line1.split()
		opr1 = words[0]
		blk_num1 = int(words[1])
		len1 = int(words[2])

	total_read = 1;
	sequential_read = 1;
	#print opr1, blk_num1, len1
	first_time = 1;

	opr2 = ''; 
 
	while True:  
		while opr2 != 'read':  
			   
			line2 = f.readline()
			if line2 == "": break
			words = line2.split()
			opr2 = words[0]
			blk_num2 = int(words[1])
			len2 = int(words[2])

		if line2 == "":break
		total_read = total_read + 1;
	 
		if (blk_num1 + len1) != blk_num2 :
			sequential_read = sequential_read + 1; 
		
		blk_num1 = blk_num2;  
		len1 = len2;  

		opr2 = ''; 
	print "======================================================="; 
	print "the total number of reads is %d" % total_read;
	print "the number of sequential reads is %d" % sequential_read;
	print "======================================================="; 
	
f.close(); 


with open(path,'r') as f:
	f.readline(); 
	f.readline();  
	opr1 = '' 
	while  opr1 != 'write' :  
		first_time = 0
		line1 = f.readline()
		if line1 == "": break
		words = line1.split()
		opr1 = words[0]
		blk_num1 = int(words[1])
		len1 = int(words[2])

	total_write = 1;
	sequential_write = 1;
	#print opr1, blk_num1, len1
	first_time = 1;

	opr2 = ''; 
 
	while True:  
		while opr2 != 'write':  
			   
			line2 = f.readline()
			if line2 == "": break
			words = line2.split()
			opr2 = words[0]
			blk_num2 = int(words[1])
			len2 = int(words[2])

		if line2 == "":break
		total_write = total_write + 1;
	 
		if (blk_num1 + len1) != blk_num2 :
			sequential_write = sequential_write + 1; 
		
		blk_num1 = blk_num2;  
		len1 = len2;  

		opr2 = ''; 
	print "=======================================================";
	print "the total number of writes is %d" % total_write;
	print "the number of sequential writes is %d" % sequential_write;
	print "=======================================================";
f.close(); 

