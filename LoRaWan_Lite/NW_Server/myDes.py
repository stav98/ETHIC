key = "\x21\x02\xf8\xaf\x88\x00\x09\x6e"

#Αρχικός πίνακας μετάθεσης για τα δεδομένα
#     b0  b1  b2  b3  b4  b5  b6  b7
PI = [58, 50, 42, 34, 26, 18, 10, 2,
#     b8  b9  b10
      60, 52, 44, 36, 28, 20, 12, 4,
      62, 54, 46, 38, 30, 22, 14, 6,
      64, 56, 48, 40, 32, 24, 16, 8,
      57, 49, 41, 33, 25, 17,  9, 1,
      59, 51, 43, 35, 27, 19, 11, 3,
      61, 53, 45, 37, 29, 21, 13, 5,
      63, 55, 47, 39, 31, 23, 15, 7]

#Αντίστροφος του αρχικού πίνακα μετάθεσης για τα δεδομένα
PI_1 = [40, 8, 48, 16, 56, 24, 64, 32,
        39, 7, 47, 15, 55, 23, 63, 31,
        38, 6, 46, 14, 54, 22, 62, 30,
        37, 5, 45, 13, 53, 21, 61, 29,
        36, 4, 44, 12, 52, 20, 60, 28,
        35, 3, 43, 11, 51, 19, 59, 27,
        34, 2, 42, 10, 50, 18, 58, 26,
        33, 1, 41,  9, 49, 17, 57, 25]

#Αρχικός πίνακας μετάθεσης για το κλειδί
CP_1 = [57, 49, 41, 33, 25, 17, 9, 8,
        1, 58, 50, 42, 34, 26, 18, 40,
        10, 2, 59, 51, 43, 35, 27, 24,
        19, 11, 3, 60, 52, 44, 36, 56,
        63, 55, 47, 39, 31, 23, 15, 16,
        7, 62, 54, 46, 38, 30, 22, 32, 
        14, 6, 61, 53, 45, 37, 29, 48,
        21, 13, 5, 28, 20, 12, 4, 64]

#Πίνακας μετάθεσης για παραγωγή κλειδιών
CP_2 = [14, 17, 11, 24, 1, 5, 3, 28,
        15, 6, 21, 10, 23, 19, 12, 4,
        26, 8, 16, 7, 27, 20, 13, 2,
        41, 52, 31, 37, 47, 55, 30, 40,
        51, 45, 33, 48, 44, 49, 39, 56,
        34, 53, 46, 42, 50, 36, 29, 32,
        64, 61, 63, 62, 57, 60, 58, 59,
        9, 18, 22, 25, 35, 38, 43, 54 ]

#Οδηγίες ολίσθησης για παραγωγή κλειδιών
SHIFT = [1,1,2,2,3,2,2,2,1,2,2,2,2,2,2,1]

#Οδηγίες ολίσθησης για κωδικοποίσης δεδομένων σε 16 γύρους
SHIFT1 = [0,1,2,2,3,4,5,6,0,-1,-2,-3,-4,-6,-5,7]

keys = list() 

#Return the binary value as a string of the given size 
def binvalue(val, bitsize):
    binval = bin(val)[2:] if isinstance(val, int) else bin(ord(val))[2:]
    if len(binval) > bitsize:
        raise "binary value larger than the expected size"
    while len(binval) < bitsize:
        binval = "0"+binval #Add as many 0 as needed to get the wanted size
    return binval

#Μετατρέπει ένα String σε μια λίστα από bits
def string_to_bit_array(text):
    array = list()
    for char in text:
        binval = "{0:08b}".format(ord(char)) #binvalue(char, 8)#Get the char value on one byte
        array.extend([int(x) for x in list(binval)]) #Πρόσθεσε τα bits στη τελική λίστα
    return array

#Χωρίζει μια λίστα σε υπολίστες με μήκος n 
def nsplit(s, n):
    return [s[k:k+n] for k in range(0, len(s), n)]

#Επαναφέρει το String από την λίστα με τα bits
def bit_array_to_string(array):
    res = ''.join([chr(int(y,2)) for y in [''.join([str(x) for x in _bytes]) for _bytes in  nsplit(array,8)]])   
    return res

#Προσθέτει padding βάσει του PKCS5
def addPadding(data):
    pad_len = 8 - (len(data) % 8)
    data += pad_len * chr(pad_len)
    return data, pad_len

#Αφαιρεί padding αν υπάρχει
def removePadding(data):
    pad_len = ord(data[-1])
    if pad_len < 8:
        return data[:-pad_len]
    else:
        return data

#Κάνει XOR δύο λίστες με bits και επιστρέφει μια νέα λίστα
def xor(t1, t2):
        return [x^y for x,y in zip(t1,t2)]

#Ολισθαίνει μια λίστα σύμφωνα με την τιμή n
def shift(g, n):
        return g[n:] + g[:n]

#Κάνει μετάθεση των bits βάσει του πίνακα table
def permut(block, table):#Permut the given block using the given table (so generic method)
        return [block[x-1] for x in table]

#Κρυπτογράφηση των δεδομένων
def encrypt(text, key):
    #global keys
    #keys = []
    if len(text) % 8 != 0:
        text, l = addPadding(text)
    else:
        l = -len(text)
    #------- Δημιουργία κλειδιών κρυπτογράφησης --------------
    k = string_to_bit_array(key)
    k = permut(k, CP_1) #Φτιάξε κλειδί βάσει του πίνακα μετάθεσης
    #kk = bit_array_to_string(k) #--
    #for i in kk:   #--
    #    print(ord(i)) #--
    for i in range(16): #Κάνε 16 γύρους
        k = shift(k, SHIFT[i]) #Shift των bit του κλειδιού και δημιουργία 16 νέων κλειδιών
        #d=(bit_array_to_string(k)) #--
        #test_list = [ord(c) for c in d] #--
        #print(test_list)
        keys.append(permut(k, CP_2)) #Προσθήκη στη λίστα κλειδιών με μετάθεση CP_2
        #kk = permut(k, CP_2)
        #keys.append(kk)

        #d=(bit_array_to_string(kk)) #--
        #test_list = [ord(c) for c in d] #--
        #print(test_list) #--
        
    #------- Kρυπτογράφηση δεδομένων ------------------------
    #Χώρισε τα δεδομένα σε blocks των 8 bytes δηλ. 64 bits
    text_blocks = nsplit(text, 8)
    text = ""
    for block in text_blocks:
        b = string_to_bit_array(block)
        b = permut(b, PI) #Αρχική μετάθεση 
        z = b
        for i in range(16): #Do the 16 rounds
            z = xor(keys[i], z)#If encrypt use Ki
            z = shift(z, SHIFT1[i])
            #d=(bit_array_to_string(z)) #--
            #test_list = [ord(c) for c in d] #--
            #print(test_list) #--
        text += bit_array_to_string(z)
    #test_list = [ord(c) for c in text] #Debug
    #print(test_list) #Debug το κωδικοποιημένο μήνυμα σε λίστα
    return text

#Αποκρυπτογράφηση των δεδομένων
def decrypt(text, key):
    #------- Αποκρυπτογράφηση δεδομένων ----------------------
    text_blocks = nsplit(text, 8)
    text = ""
    for block in text_blocks:
        b = string_to_bit_array(block)
        z = b
        for i in range(16): #Do the 16 rounds
            z = shift(z, -SHIFT1[15-i])
            z = xor(keys[15-i], z)#If encrypt use Ki
        z = permut(z, PI_1)
        text += bit_array_to_string(z) #z
    return text

#==========================================================================
