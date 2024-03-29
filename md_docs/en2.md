<a href="../README.md"><= Επιστροφή</a><br>

<p>Το υλικό ελέγχου έχει χωριστεί σε τρία μέρη:</p>
  <b>1. Κεντρικός διακομιστής</b>
  <p>Ο διακομιστής μας είναι ένα Raspberry PI 4 στο οποίο θα εγκατασταθούν τα πακέτα: Mosquitto MQTT broker, Node Red και <a href="/LoRaWan_Lite/NW_Server" target="_blank">Network Server</a> το οποίο γράψαμε σε Python 3. Το Raspberry μπορεί να βρίσκεται οπουδήποτε αρκεί να υπάρχει σύνδεση με το διαδίκτυο, να έχουν ανοίξει οι συγκεκριμένες πόρτες του router και να κατευθύνονται στο Raspberry. Επίσης αν έχουμε δυναμική απόδοση διεύθυνσης IP, πρέπει να φαίνεται στον έξω κόσμο μέσω κάποιου παρόχου δυναμικού DNS π.χ. FreeDNS, DynDNS κλπ. Εναλλακτικά μπορούμε να βάλουμε ένα PC με Linux ή να νοικιάσουμε κάποιο VPS με Linux.</p>
  <b>2. Πύλη επικοινωνίας LoRa</b>
  <p>Η <a href="/LoRa_GateWay">πύλη LoRa</a> είναι μια διαφανής πύλη η οποία λαμβάνει πλαίσια LoRa WAN από τους ελεγκτές θερμοκηπίων - κόμβους και τα προωθεί στο διαδίκτυο και συγκεκριμένα στο Raspberry Pi 4 ή τον διακομιστή τον οποίο στήσαμε στην προηγούμενη παράγραφο. Επίσης παίρνει πακέτα από τον διακομιστή και τα προωθεί στους ελεγκτές - κόμβους. Η πύλη τοποθετείται οπουδήποτε αρκεί να έχει πρόσβαση στο διαδίκτυο. Επειδή τα θερμοκήπια μπορεί να είναι αρκετά μακριά, η πύλη μπορεί να εγκατασταθεί στο σπίτι του παραγωγού.</p>
  <b>3. Ελεγκτής θερμοκηπίου - Κόμβος</b>
<p>Οι κόμβοι μπορούν να βρίσκονται σε διαφορετικά μέρη και σε απόσταση μέχρι 20Km (με εξωτερικές κεραίες) από την πύλη αρκεί να μην υπάρχουν πολλά φυσικά εμπόδια όπως κτίρια ή βουνά.</p>

<a href="../README.md"><= Επιστροφή</a><br>
