// Here are settings you have to change to adapt module to your specific needs
// Voici les paramètres que vous devez changer pour adapter le module à vos besoins

//  *** Options ***
#define SERIAL_PORT Serial                          // Indicates which serial to use (don't define if no serial output needed)
                                                    // Indique le port série à utiliser (ne pas définir si on ne veut pas de sortie série)
uint8_t openPin = D5;                               // Pin where open relay is connected to
                                                    // Pinoche où le relai ouverture est connecté
uint8_t closePin = D6;                              // Pin where close relay is connected to
                                                    // Pinoche où le relai fermeture est connecté
uint8_t chickenDetectionPin = D7;                   // Pin where chicken detector is connected to
                                                    // Pinoche où le détecteur de poule est connecté
uint8_t ldrPin = A0;                                // Pin where LDR is connected to
                                                    // Pinoche où le la LDR est connectée

//  *** Wifi stuff ***
//  *** Connection WiFi ***
char wifiSSID[] = "MySSID";                         // SSID to connected to
                                                    // SSID auquel se connecter
char wifiKey[] = "MyPassword";                      // SSID password
                                                    // Mot de passe associé au SSID
char nodeName[] = "chickenDoor";                    // ESP node name
                                                    // Nom du module ESP

//  *** MQTT client ***
//  *** Client MQTT ***
char mqttServer[] = "192.168.1.1";                  // MQTT server IP address or name
                                                    // Adresse IP ou nom du serveur MQTT
uint16_t mqttPort = 1883;                           // MQTT IP port
                                                    // Port IP du serveur MQTT
char mqttUsername[] = "MyMqttUsername";             // MQTT username
                                                    // Utilisateur MQTT
char mqttPassword[] = "MyMqttPassword";             // MQTT password
                                                    // Mot de passe MQTT
char mqttCommandTopic[] = "chickenDoor/command";    // MQTT command topic, where commands are received
                                                    // Sujet MQTT où les commandes sont reçues
char mqttStatusTopic[] = "chickenDoor/status";      // MQTT status topic, where status is sent to (and read at startup)
                                                    // Sujet MQTT où les états sont envoyés (et lus au lancement)
char mqttSignalTopic[] = "chickenDoor/signal";      // MQTT signal topic, where information and error messages are sent to
                                                    // Sujet MQTT où les messages d'information ou d'erreur sont envoyées
char mqttSettingsTopic[] = "chickenDoor/settings";  // MQTT settings topic, where settings are read (and sent on request)
                                                    // Sujet MQTT où les paramètres sont lus (et écrits sur demande)

//  *** Time zone ***
//  *** Fuseau horaire ***
int8_t timeZone = 1;                                // (Signed) hour difference between GMT time and local time in winter (-12 to +14)
                                                    // Nombre d'heures (signé) entre l'heure GMT et l'heure locale en hiver (-12 à + 14)
int8_t timeMinutes = 0;                             // (Signed) additional minutes to add or substract (-59 to +59)
                                                    // Nombre de minutes additionnelles (signé) à ajouter ou retirer (-59 à +59)
uint8_t dstZone = 0;                                // DST zone (0=Europe, 1 = USA)
                                                    // Type de changement horaire d'été (0=Europe, 1 = USA)
bool dayLight = true;                               // Follow day light saving time? 
                                                    // Heure d'hiver/d'été utilisée ?

//  *** Sun parameters ***
//  *** Paramètres du soleil ***
float latitude = 45;                                // Latitude of chicken door
                                                    // Latitude du poulailler
float longitude = -1.5;                             // Longitude of checken door
                                                    // Longitude du poulailler
float zenith = JC_Sunrise::civilZenith;             // Type of zenith. Can be:
                                                    // Type de zenith à utiliser. Peut être :
                                                    //      officialZenith (90.83333)
                                                    //      civilZenith (96)
                                                    //      nauticalZenith (102.0)
                                                    //      astronomicalZenith (108.0)

//  *** Settings (can be changed through mqttSettingsTopic) ***
//  *** Paramètres (peuvent être changés au travers du sujet MqttSettingsTopic) ***
uint16_t openLuminosity = 1023;                     // Luminosity required to open door (0-1023)
                                                    // Luminosité de l'ouverture de porte (0-1023)
uint16_t closeLuminosity = 0;                       // Luminosity required to close door (0-1023)
                                                    // Luminosité de la fermeture de porte (0-1023)
uint16_t openDuration = 32000;                      // Normal duration of door opening (in ms)
                                                    // Durée normale de l'ouverture de porte (en ms)
uint16_t closeDuration = 32000;                     // Normal duration of door closing (in ms)
                                                    // Durée normale de la fermeture de porte (en ms)
int16_t endOfCourseCurrent = -10;                   // Motor's current at end of course (mA), 
                                                    //      positive value means test current greater than this value (motor blocked),
                                                    //      negative means test current lower than this absolute value (end of course switch cutted power)
                                                    // Courant moteur en fin de course
                                                    //      une valeur positive indique que le test se fait sur une valeur supérieure (moteur bloqué)
                                                    //      une valeur négative indique que le test se fair sur une valeur absolue inférieure (fin de course par coupure moteur)
uint16_t obstacleCurrent = 800;                     // Motor's current indicating that something is blocking door (normal mouvment is less than this) (in mA)
                                                    // Courant moteur indiquant que quelque chose bloque le moteur (le mouvement normal est inférieur à cette valeur) (en mA)
int16_t sunOffsetMinutes = 0;                       // (Signed) minutes to add on sun rise and remove on sun set
                                                    // Nombre de minutes (signé) à ajouter au lever du soleil et à retrancher au coucher du soleil
