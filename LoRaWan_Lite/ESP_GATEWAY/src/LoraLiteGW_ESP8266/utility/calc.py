PL = 48 #Payload Bytes (1-255)
#PL += 4 #Γιατί έχω extra διευθύνσεις(2), μήκος(1), ID(1)
ACK = 11
ACK += 4

#--------------------------------------------------
SF = 9 #10 #Default 7
CR = 4 / 5 #Default 4/5
cr = 1 #1 για 4/5, 2 για 4/6, 3 για 4/7, 4 για 4/8
BW = 125000 #Hz Default - (-121dBm)
#BW = 62500 #62500 #Hz (-124dBm)
#--------------------------------------------------
th = 0
if BW == 62500:
    th_base = 121
elif BW == 125000 or BW == 250000:
    th_base = 118
    
for o,i in enumerate([6,7,8,9,10,11,12], 0):
    if i == SF:
        th = th_base + (o * 3)

N_Preable = 8 #Default
IH = 0 #0, Δηλαδή υπάρχει Header, 1 αν δεν υπάρχει
DE = 0 #0, 1 αν υπάρχει LowDataRateOptimize
CRC = 1 #1 υπάρχει CRC στο Payload

print("Thresshold= -", th,"dBm", sep='')
Rs = (BW / 2 ** SF)
Br = Rs * SF * CR
Ts = 1 / Rs
print("Rs = ", Rs, "Symbols/sec")
print("Bit Rate = ", Br, "bps")
print("Symbol time = ", Ts * 1000, "ms")

def calc(pl):
    Tpre = (N_Preable + 4.25) * Ts
    print("Preable Time = ", Tpre * 1000, "ms")
    Npl = 8 + max(round((8 * pl - 4 * SF + 28 + 16 * CRC - 20 * IH)/(4 * (SF - 2 * DE)), 0) * (cr + 4), 0)
    print("Number of payload symbols = ", Npl)
    Tpl = Npl * Ts
    print("Payload Time = ", Tpl * 1000, "ms")
    Tpacket = Tpre + Tpl
    print("================================================")
    print("Packet Time on Air = ", Tpacket * 1000, "ms")
    return Tpacket * 1000

#tot = int(round(calc(PL) + calc(ACK), 0))
#tot = round(calc(PL) + calc(ACK), 5)
tot = round(calc(PL), 5) 
print("Total time =", tot, "ms")
x = int(round(((PL * 1000) / tot) * 8, 0))
print("Real bitrate: ", x, "bps")
