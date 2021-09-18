#Mosquitto
#mosquitto_sub -h host -t lora/node1_out -u user -P password
#mosquitto_pub -h host -m "LED1=On" -t lora/node1 -u user -P password -d

import socket, json
import binascii, random
import time
from threading import Thread
import paho.mqtt.client as mqtt
from myDes import *

broker_address = "example.com"
broker_portno = 1883
client = mqtt.Client()
client.username_pw_set(username="user", password="password")

#------- Διεύθ.   Topic         Topic out
node1 = [0xab31, "lora/node1", "lora/node1_out"]
node2 = [0xab33, "lora/node2", "lora/node2_out"]

nodes = [node1, node2]

node_msgs = []

def on_message(client, obj, msg):
    print(msg.topic + " " + str(msg.qos) + " " + str(msg.payload))
    for node in nodes:
        if msg.topic == node[1]:
            node_msgs.append([node[0], str((msg.payload), 'latin-1')])
            break
    print(node_msgs)

def on_connect(client, obj, flags, rc):
    print("MQTT_rc: " + str(rc))    

def on_subscribe(client, obj, mid, granted_qos):
    print("MQTT_Subscribed: " + str(mid) + " " + str(granted_qos))

def on_publish(client,userdata,result):             #create function for callback
    print("MQTT: data published")
    pass

client.on_connect = on_connect
client.on_message = on_message
client.on_subscribe = on_subscribe
client.on_publish = on_publish
client.connect(broker_address, broker_portno)
for node in nodes:
    client.subscribe(node[1])
client.loop_start()

AppIsRunning = True

localIP     = "0.0.0.0"  #"127.0.0.1"
localPort   = 1700
bufferSize  = 1024
NbCount     = 4 #Αριθμός επανεκπομπών αν δεν πάρει Ack. Η τελευταία είναι στο RX2

rx_time = 0
Tx1_delay = 1000000
Tx2_delay = 2000000

sf1 = 'SF7BW125'
sf2 = 'SF12BW125'
sf = ''

node_addr1 = node_addr2 = 0
fcntDown = 0
Ack_Ok = True
msgFromServer = ""
RecentTransAckReq = []
Ack_Requested = False
Retrans_cnt = 0

#-------------                    1         2         3         4         5         6         7         8         9        10        11        12  
#-------------           12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678
#msgFromServer1       = "Hello lora Node from NW Server 123@#!.,st ------------------- $ &&&&&& **** @@@@@@@@@@@@@@@@@@@@@@ . ++++++++++++++++ *********."
#msgFromServer2       = "Stavros Lora Lite Network"
#msgFromServer3       = "Papaki @ paei sth potamia !!!!! ."
#node_msgs.append([node1, msgFromServer1])
#node_msgs.append([node2, msgFromServer1])
#node_msgs.append([node1, msgFromServer2])
#node_msgs.append([node1, msgFromServer3])

#gwAddressPort   = ("192.168.42.29", 1700)

# Create a datagram socket
UDPServerSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

# Bind to address and ip
UDPServerSocket.bind((localIP, localPort))

#UDPClientSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

print("\n\n********** LoRa Lite Server is up and running. **********\n\n")

TX_PK = {'txpk':[{'imme':False,'tmst':0, 'time':'','rfch':0,'freq':0.0,'datr':'','codr':'','power':17,'ipol':False,'size':0,'data':''}]}

#Ετοιμάζει πακέτο για τον Node και το στέλνει στη GW. Αυτή θα το μεταδώσει στο ερχόμενο RX window
#Μπορεί να είναι μόνο ACK αν ζητάει επιβεβαίωση το Node και δεν έχει κάτι άλλο να στείλει
def pull_resp(conf_ack, TxWindow):
    global fcntDown, msgFromServer, RecentTransAckReq, Ack_Requested, Retrans_cnt
    print("Pull Response")
    #Προετοιμασία του LoRa Frame που θα αναμεταδώσει η GW
    buff = []
    #Κατασκευή FHDR (Frame Header)
    if conf_ack: #Αν η παράμετρος είναι True τότε θα θέσει το bit ACK = 1
        ack = 16
    else:
        ack = 0
    Ack_Requested = False
    for i in range(len(RecentTransAckReq)):
        if RecentTransAckReq[i] == node_addr2 * 256 + node_addr1:
            RecentTransAckReq.pop(i)
            Ack_Requested = True
            break
    #Αν πήρε ACK ή δεν χρειαζόταν ACK ή προσπάθησε NbCount φορές τότε πάρε το επόμενο μήνυμα από την ουρά
    if Ack_Ok or not Ack_Requested or Retrans_cnt >= (NbCount - 1):
        msgFromServer = ""
        Retrans_cnt = 0
        fcntDown += 1 #Αύξησε τον μετρητή πλαισίου κατά 1
        if fcntDown > 65535: #Αν περάσει τα 16bit να μηδενιστεί 
            fcntDown = 0
        #Διάβασε την ουρά με τα μηνύματα για τους κόμβους
        for i in range(len(node_msgs)):
            item = node_msgs[i]
            if item[0] == node_addr2 * 256 + node_addr1: #Αν υπάρχει μήνυμα για τον κόμβο με αυτή την διεύθυνση
                msgFromServer = item[1] #Πάρε το μήνυμα
                node_msgs.pop(i) #Βγάλτο από την ουρά
                break #Σταμάτα την επανάληψη
    #Διαφορετικά το μήνυμα θα σταλεί ξανά
    else:
        Retrans_cnt += 1
    if len(msgFromServer) > 0: #Αν υπάρχει αποθηκευμένο μήνυμα για τον node, τότε να ζητηθεί επιβεβαίωση 
        print("Downlink (" + str(Retrans_cnt + 1) + ") ====================>", msgFromServer) #Debug
        buff.append((128 + 64) | ack) #Μήνυμα Downlink (από πύλη προς κόμβο) 64 για Downlink 128 για αίτημα επιβεβαίωσης λήψης από κόμβο
        RecentTransAckReq.append(node_addr2 * 256 + node_addr1) #Βάλτο στην ουρά ώστε την επόμενη φορά να ξέρεις αν απαιτείται επιβεβαίωση
    else: #Αλλιώς θα στείλει μόνο ack και δεν θα ζητηθεί επιβεβαίωση
        print("Downlink =============> Ack only") #Debug
        buff.append(64 | ack)
    #Φτιάξε την διεύθυνση παραλήπτη που είναι ίδια με αυτή του μηνύματος rxpk
    buff.append(node_addr1); buff.append(node_addr2); buff.append(fcntDown & 255); buff.append((fcntDown >> 8) & 255); buff.append(1)
    #print(buff)
    t = encrypt(msgFromServer, key) #Κωδικοποίηση απλοποιημένου DES του μηνύματος και επιστρέφει text
    t = t.encode('latin-1') #Να γίνει bytes Latin-1 !!! Προσοχή σημαντικό για Arduino
    pl = list(t) #Να γίνει λίστα
    #print(pl) #Debug δεκαδικό
    #print([hex(x) for x in pl]) #Debug Hex
    ss = buff + pl #Να ενωθούν οι δύο λίστες
    #---- Υπολογισμός του Checksum ------
    s = 0
    for i in range(0, (len(ss)), 2):
        s = s + (ss[i] * 256 + ss[i + 1])
        if s > 65535:
            s = s & 65535
            s += 1
    #Το Checksum είναι XOR με την διεύθυνση κόμβου
    ss.append((s & 255) ^ buff[1]) #Βάλτο στο τέλος του frame, πρώτα το LSB
    ss.append(((s >> 8) & 255) ^ buff[2]) #και μετά το MSB τελευταίο
    #Debug εμφάνιση τελικού μηνύματος σε hex
    '''
    dh = "".join("%02X " % i for i in ss) #Μετατροπή σε Hex String Debug
    print(dh) #Debug
    '''
    t = bytes(ss) #Ξαναγίνεται bytes
    size = len(t)
    t = binascii.b2a_base64(t) #Να γίνει BASE64 μορφή bytes b'......
    TX_PK["txpk"][0]["data"] = t.decode()[:-1] #Βάλε τα data σαν string όχι bytes (μετατροπή από b'..... σε string) και κόψε το τελευταίο \n
    TX_PK["txpk"][0]["size"] = size
    TX_PK["txpk"][0]["ipol"] = True
    #Επιλογή παραθύρου εκπομπής 1 ή 2
    if TxWindow == "TX1" and Retrans_cnt < (NbCount - 1):
        t1 = int(rx_time) + Tx1_delay
        sf = sf1
    #elif TxWindow == "TX2":
    else:
        t1 = int(rx_time) + Tx2_delay
        sf = sf2
    #Αν ο αριθμός υπερβεί τα 32bit να πάει από την αρχή
    if t1 > 4294967295:
        t1 = t1 - 4294967296
    TX_PK["txpk"][0]["tmst"] = t1
    TX_PK["txpk"][0]["datr"] = sf
    pj = json.dumps(TX_PK)
    data = bytes([0x01, 0x00, 0x00, 0x03]) + pj.encode() #Τελικά δεδομένα για αποστολή
    print(data) #Debug
    UDPServerSocket.sendto(data, address)
    #UDPClientSocket.sendto(data, gwAddressPort)

    '''
    #Αποκωδικοποίηση για δοκιμή και Debugging
    payload = data[4:].decode("utf-8") #Το json μήνυμα ξεκινάει από την θέση 4
    #print("Data", payload)
    #Αν είναι txpk να αποκωδικοποιηθεί
    if 'txpk' in payload:
        tx_pk = json.loads(payload) #Parsing το πακέτο Json
        tx_data = tx_pk["txpk"][0]["data"]
        #print(tx_data) #Debug
        dec_bin = binascii.a2b_base64(tx_data) #Να γίνει bytes από Base64
        dec_bin = dec_bin.decode() #Να γίνει str UTF-8
        dec_bin = dec_bin.encode('latin-1') #Να γίνει bytes latin
        pckt_bytes = list(dec_bin) #Κάνε όλο το πακέτο μια λίστα ακεραίων
        #print(pckt_bytes) #Debug
        dec_bin = dec_bin.decode('latin-1') #Μετατροπή από bytes σε String latin-1
        dec = decrypt(dec_bin, key) #Αποκωδικοποίηση βάσει κλειδιού
        print("Deconding: ", removePadding(dec)) #Αφαίρεση padding
     '''

#Χρονιστής ο οποίος σε τακτά χρονικά διαστήματα στέλνει αιτήματα Pull_Resp 
#def Timer1(seconds):
#    while AppIsRunning:
#        time.sleep(seconds)
#        pull_resp()
        
#------- Δημιουργία κλειδιών αποκρυπτογράφησης ----------
keys = []
k = string_to_bit_array(key)
k = permut(k, CP_1)
for i in range(16):#Apply the 16 rounds
    k = shift(k, SHIFT[i])
    keys.append(permut(k, CP_2))

#myThread = Thread(target=Timer1, args=(65,)) #Ετοιμάζει το Thread για λειτουργία κάθε n seconds
#myThread.start() #Ξεκινάει το Thread

#ms_now = int(time.time()*1000) #Χρόνος σε msec

#-------- Συνεχόμενη ακρόαση για εισερχόμενα datagrams ----------------
try:
    while(True):
        bytesAddressPair = UDPServerSocket.recvfrom(bufferSize)
        message = bytesAddressPair[0]
        address = bytesAddressPair[1]
        clientMsg = "Message from GW:{}".format(message)
        clientIP  = "Client IP Address:{}".format(address)
        print(clientMsg) #Debug
        print(clientIP)  #Debug
        #Λήφθησαν PULL_DATA από GW
        if message[3] == 0x02: #Απάντηση με PULL_ACK  
            data = bytes([0x01, message[1], message[2], 0x04]) + message[4:]
            UDPServerSocket.sendto(data, address)
            print("Send response")
        #Λήφθησαν PUSH_DATA από GW
        elif message[3] == 0x00:
            print("GW-Address:", hex(message[4]), ":", hex(message[5]), ":", hex(message[6]), ":", hex(message[7]), ":", hex(message[8]), ":", hex(message[9]), ":", hex(message[10]), ":", hex(message[11])) 
            payload = message[12:].decode("utf-8") #Το json μήνυμα ξεκινάει από την θέση 12
            #print("Data", payload) #Debug
            #Αν είναι rxpk να αποκωδικοποιηθεί
            if 'rxpk' in payload:
                rx_pk = json.loads(payload) #Parsing το πακέτο Json
                rx_data = rx_pk["rxpk"][0]["data"]
                rx_time = rx_pk["rxpk"][0]["tmst"] #Ο χρόνος που τελείωσε η λήψη του πακέτου LoRa από την πύλη σε μsec
                print("Tmst", rx_time)
                #print(rx_data) #Debug
                dec_bin = binascii.a2b_base64(rx_data) #Να γίνει bytes από Base64
                pckt_bytes = list(dec_bin) #Κάνε όλο το πακέτο μια λίστα ακεραίων
                #---- Υπολογισμός του Checksum ------
                s = 0
                for i in range(0, (len(pckt_bytes) - 2), 2):
                    s = s + (pckt_bytes[i] * 256 + pckt_bytes[i+1])
                    if s > 65535:
                        s = s & 65535
                        s += 1
                #---- Πάρε το Checksum που έστειλε ο κόμβος ---
                cs = dec_bin[-2:] #Πάρε τα 2 τελευταία bytes που είναι το Check Sum
                cs = int.from_bytes(cs, "little") #Να γίνει ακέραιος
                cs = cs ^ dec_bin[1] + dec_bin[2] * 256
                #Αν τα δύο Checksums συμφωνούν τότε απάντησε με PUSH_ACK
                if cs == s:
                    node_addr1 = dec_bin[1]; node_addr2 = dec_bin[2] #Πάρε το ID του κόμβου ο οποίος έστειλε το μήνυμα
                    if dec_bin[0] & 16: #Αν υπάρχει Ack στο πλαίσιο Upload σημαίνει ότι το έλαβε ο κόμβος
                        Ack_Ok = True
                    else:
                        Ack_Ok = False
                    
                    if dec_bin[0] & 128: #Το frame από τον κόμβο απαιτεί επιβεβαίωση λήψης
                        pull_resp(True, "TX1") #Στείλε αμέσως πακέτο προς τον node και βάλε το flag επιβεβαίωσης 1
                        #pull_resp(True, "TX2")
                    else:
                        pull_resp(False, "TX1") #Στείλε αμέσως πακέτο προς τον node
                    #Στείλε απάντηση στην GW PUSH_ACK
                    data = bytes([0x01, message[1], message[2], 0x01])
                    UDPServerSocket.sendto(data, address)
                    print("*** Send response ***")
                    #----- Αποκρυπτογράφηση -------
                    dec_bin = dec_bin[6:-2] #Αφαίρεσε το Head και Checksum και κράτα μόνο το κρυπτογραφημένο μήνυμα
                    #dh = "".join("%02X " % i for i in dec_bin) #Μετατροπή σε Hex String Debug
                    #print(dh) #Debug
                    dec_bin = dec_bin.decode("latin-1") #Μετατροπή από bytes σε String
                    dec = decrypt(dec_bin, key) #Αποκωδικοποίηση βάσει κλειδιού
                    #print("Deconding: ", removePadding(dec)) #Αφαίρεση padding
                    msg = removePadding(dec)
                    for node in nodes:
                        if (node_addr2 * 256 + node_addr1) == node[0]:
                            result = client.publish(node[2], msg)
                            status = result[0]
                            if status == 0:
                                print(f"MQTT:Send `{msg}` to topic `{node[2]}`")
                            else:
                                print(f"MQTT:Failed to send message to topic {node[2]}")
                else:
                    print("*** CheckSum Error ***")
        #Είναι TX_ACK από GW ως απάντηση πακέτου PULL RESP
        elif message[3] == 0x05:
            if message[4] == 0:
                print("Correct TX_ACK Received from GW")
except KeyboardInterrupt:
        AppIsRunning = False
