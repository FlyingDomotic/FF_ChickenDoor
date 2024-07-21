# Chicken door control/Contrôle d'une porte de poulailler

[English version and French version in the same document]

Open and close a chicken coop door depending on sun phases and/or external illumination (with optional network connection)

[ Versions françaises et anglaises dans le même document]

Ouvre et ferme la porte d'un poulailler en fonction des phases du soleil et/ou de la luminosité externe (avec une connexion réseau optionnelle)

## What's for?/A quoi ça sert ?

For those having a chicken coop, opening and closing door every morning/evening is not so fun. You can (as I did) buy a system with a small LDR (Light Dependant Resistance) to open/close door depending on illumination. however, as this is an external system, rain and condensation will oxide contacts over time, in particular those soldering LDR with connecting rods. When it happen, light is not properly detected, and door not operated anymore.

In addition, this system (at least mine), is not connected, so automation don't know what's door status.

That's why I decided to implement a small module, giving flexibility in door opening/closing. To make it short, you can open/close door by either illumination, sun phases (sunrise/sunset +/- offset) and/or external commands. In addition, an IR detector placed perpendicularly of door can detect an entering/exiting chicken, and avoid crushing it, while a current detector can detect a blocked door, either by a chicken or a external object through door path. End of course can be detected by a current drop (when contacts are curring power when reaching target) or increase (when motor is blocked at course end). Lastly, we can also detect too long opening/closing phases.

Last but not least, you can have a feedback about door status, alarm status and even opening percentage and  power/battery voltage.

Everything (settings, status and command) is done through MQTT, allowing to connect to any automation systems.

Ceux qui ont un poulailler savent qu'ouvrir et fermer la porte chaque matin/soir n'est pas la chose la plus excitante. Vous pouvez (comme je l'ai fait) acheter un système avec une LDR (résistance dépendant de la lumière) pour ouvrir/fermer la porte en fonction de la luminosité. Cependant, comme c'est un système à l'extérieur, la pluie et la condensation finissent par oxyder les contacts, en particulier ceux soudant la LDR à ses tiges. Lorsque ça arrive, la lumière n'est plus correctement détectée, et la porte reste figée.

De plus, le système (à minima le mien) n'est pas connecté, ma domotique n'a aucune idée de l'état de la porte.

C'est pourquoi, n'écoutant que mon courage qui ne disait rien (Pierre DAC), j'ai décidé de créer un petit module qui donne de la flexibilité dans l'ouverture de cette porte. Pour faire court, on peut ouvrir/fermer la porte du poulailler en fonction de la luminosité, des phases du soleil (lever/coucher +/- offset) et/ou des commandes externes. De plus, un détecteur IR placé perpendiculairement à la porte peut détecter une poule qui entre/sort, et éviter de l'écraser, pendant qu'un détecteur de courant détecte un blocage de la porte, par une poule ou un objet externe dans le chemin de la porte. La fin de course est détectée aussi bien par une chute de courant (quand des contacts de fin de course isolent le moteur en butée) ou par une augmentation du courant (quand le moteur se bloque dans la butée). Enfin, on détecte des phases d'ouvertures/fermetures trop longs.

De plus, on a un retour d'état de la porte, des alarmes et même du pourcentage d'ouverture et de la tension de l'alimentation ou de la batterie.

## Hardware/Matériel

Module is build around an ESP8266, associated to a pair of relay (to power motor) and an INA219 current detector. Optionally, you can add a LDR (to detect illumination) and an IR detector like E3F-DSP30D1 (to detect a chicken crossing door). An optional DS1307 RTC module allow getting time when an NTP server is not available at startup time.

Le module est construit autour d'un ESP8266, d'une paire de relais (pour alimenter le moteur) et d'un détecteur de courant de type INA219. On peut optionnellement ajouter une LDR (pour détecter la luminosité) et un détecteur infra-rouge genre E3F-DSP30D1(pour détecter une poule entrante/sortante. Un module DS1307 optionnel permet d'avoir la date et l'heure si un serveur NTP n'est pas disponible au démarrage de l'ESP. 

## Network/Réseau

Module can work autonomously, without network (in this case, only illumination can be used, sun related time and external commands are not available). But you can use ESP8266 WiFi chip, to connect it to you network, and being able to manage/monitor it.

Le module peut fonctionner de façon autonome, sans réseau (dans ce cas, seul la luminosité peut être utilisée, les horaires liées au soleil et les connexions externes ne sont pas disponibles). Mais vous pouvez avantageusement utiliser la connexion Wifi de l'ESP8266 pour le connecter à votre réseau, et être capable de le commander/monitorer.


## Prerequisites/Prérequis

VSCodium (or Visual Studio Code) with PlatformIO should be installed. You may also use Arduino IDE, as long as you read platforio.ini file to get the list of required libraries.

Vous devez avoir installé VSCodium (ou Visual Studio Code) avec PlatformIO. Vous pouvez également utiliser l'IDE Arduino, à condition d'extraire la liste des librairies requises indiquée dans le fichier platformio.ini.

## Installation

Follow these steps:

1. Clone repository in folder where you want to install it
```
cd <where_ever_you_want>
git clone https://github.com/FlyingDomotic/chickenDoor.git chickenDoor
```
2. Make ESP connections (have a look at chickenDoor shema.jpg):
	- Motor's relays connected to D5 (open) and D6 (close). On contact side, connect positive motor's wire (the one which opens door when connected to positive lead) to D5 relay central position, negative motor's wire (the one which opens door when cpnnected to ground) on D6 relay central position. Connect both relays to power ground on normally closed contact and INA219 V- on normally open.
	- INA219 should be connected to D1 (SCL) and D2 (SDA). In addition, power ground should be connected on GND and power positive voltage to V+. Connect VCC to +3.3V.
	- Optional DS1307 connected to D1 (SCL) and D2 (SDA). In addition, power ground should be connected on GND and power positive voltage to V+. Connect VCC to +3.3V. You should then remove (desolder) R2 and R3 I2C resistor as they pull SDA and SCL up. They're located just under the 2 ICs.
	- Optional LDR connected between A0 and +3.3V and a 100K pull-up resistor between A0 and ground (leave it unconnected if not used)
	- Optional IR detector to D7 (leave not connected if not used).
3. Change settings to map your needs (see here under).
4. Compile and load code into ESP.
5. Use it!

If needed, you can connect on serial/USB ESP link to see debug messages (at 74880 bds).

Suivez ces étapes :

1. Clonez le dépôt GitHub dans le répertoire où vous souhaitez l'installer
```
cd <là_où_vous_voulez_l'installer>
git clone https://github.com/FlyingDomotic/chickenDoor.git chickenDoor
```
2. Connecter l'ESP (jetez un oeil sur le fichier chickenDoor schema.jpg) :
	- Les relais du moteur sont connectés sur D5 (ouverture) et D6 (fermeture). Du coté contacts, le fil positif du moteur (celui qui ouvre la porte lorsqu'il est connecté au positif de l'alimentation) doit être connecté sur la position centrale du relai D5, le fil négatif du moteur (celui qui ouvre la porte lorsque connecté au négatif de l'alimentation) sur le point central du relai D6. Connecter le contact  normalement fermé de chaque relai à la masse, et le normalement ouvert à la borne V6 de l'INA219.
	- L'INA219 doit être connecté à D1 (SCL) et D2 (SDA). De plus, la masse de l'alimentation doit être connectée à GND, et le pole positif de l'alimentation du moteur sur V+. Connecter VCC au +3.3V.
	- le DS1307 (optionnel) doit être connecté à D1 (SCL) et D2 (SDA). De plus, la masse de l'alimentation doit être connectée à GND, et le pole positif de l'alimentation du moteur sur V+. Connecter VCC au +3.3V. Vous devez enfin retirer (sésouder) les résistances R2 et R3 qui tirent à l'état haut le bus I2C. Elles sont juste en dessous des 2 circuits intégrés.
	- la LDR (optionnelle) doit être connectée entre A0 et le +3.3V et une résistance de tirage de 100 K entre A0 et la masse (laisser non connecté si la LDR n'est pas utilisé).
	- Le détecteur IR (optionnel) doit être connecté sur D7 (à laisser non connecté sui pas utilisé).
3. Modifier le paramétrage pour qu'il corresponde à vos besoins (voir plus bas).
4. Compiler et charger le code dans l'ESP.
5. Utilisez-le !

Si besoin, vous pouvez vous connecter sur le lien série/USB de lESP pour voir les messages de déverminage (à 74880 bds).

## Settings/Paramètres

Settings are given in chickenDoorParameters.h. You have to edit it to give your own settings before compiling project. Details of each parameter is given in comments of this file.

You have to recompile project, and reload firmware should you change chickenDoorParameters.h file.

You may also change them using MQTT. In this case, give the "settings" command to get current settings, change parameters you want to modify and resend it to MQTT settings topic. If you omit some settings, previous values will be retained. Again, giving "settings" command will push all settings into MQTT settings topic. This is a good idea, when everything runs well, to make changes back into chickenDoorParameters.h en ensure next compilation will be done with right settings.

Le paramétrage se fait dans le fichier chickenDoorParameters.h. Vous devez l'éditer et y mettre vos propres valeurs avant de compiler le code. Le détail de chaque paramètre figure dans les commentaires de ce fichier.

Vous devez ensuite recompiler le projet et recharger le firmware quand vous modifiez chickenDoorParameters.h.

Vous pouvez aussi les changer au travers de MQTT. Dans ce cas,passez la commande "settings" pour récupérer les valeurs courantes, changez le(s) paramètre(s) que vous souhaitez modifier, et renvoyez-les dans le sujet MQTT des paramètres. Encore une fois, la commande "settings" va récupérer les paramètres actifs dans le sujet paramètres. Il est habile, une fois que tout tourne rond, de reporter les changements dans chickenDoorParameters.h pour s'assurer que la prochaine compilation prendra les bons paramètres.

## Available commands/Commandes disponibles

You may send the following commands through MQTT command topic. Response will be done through the same topic, adding either "done" or "unknown" to the command. In addition, depending on command, you may also get data through signal, status or settings topic.

Vous pouvez envoyer les commandes suivantes au sujet MQTT commande. La réponse sera faite dans le même sujet, en ajoutant "done" ou "unkown" à la commande envoyée. De plus, en fonction de la commande, vous pouvez également recevoir des données dans les sujets d'information, d'état ou de paramétrage.

	- "manualclose": close the door and activate manual mode/ferme la porte et passe en mode manuel.
	- "close": close the door and activate forced mode/ferme la porte et passe en mode forcé.
	- "manualopen": open the door and activate manual mode/ouvre la porte et passe en mode manuel.
	- "open": open the door and activate forced mode/ouvre la porte et passe en mode forcé.
	- "stop": stops door at current position and activate manual mode/arrête la porte dans la position courante et passe en mode manuel.
	- "auto": activate automatic mode/active le mode automatique.
	- "status": send back current status in MQTT status topic./envoie l'état actuel dans le sujet MQTT status.
	- "settings": send back current settings in MQTT settings topic./envoie les paramètres courants  dans le sujet MQTT paramètres.

Note: in manual mode, only commands will allow to change door position. In automatic mode, light and/or sun position will move door. Forced mode will be cleared when light/sun will be in phase with current position./En mode manuel, seules les commandes reçues vont changer la position de la porte. En mode automatique, la luminosité et/ou la position du soleil vont déplacer la porte. Le mode forcé sera désactivé lorsque la luminosité et/ou la position du soleil seront en phase avec l'état de la porte.

## MQTT topic contents/Contenu des sujets MQTT

	- mqttCommandTopic: MQTT command topic, where commands are received. Write here commands to send to module. It'll write back original command followed by "done" or "unknown"/Sujet MQTT où les commandes sont reçues. Écrivez ici les commandes à envoyer au module. Il va écrire en retour la commande originale suivie de "done" (fait) ou "unknown" (inconnu).
	- mqttStatusTopic: MQTT status topic, where status is sent to (and read at startup). Data is in JSON format with following fields/sujet MQTT où les états sont envoyés (et lus au lancement). Les données sont au format JSON, avec les champs suivants :
		+ "doorState" = door state/état de la porte (0=Unknown/Inconnu, 1=Closed/Fermé, 2=Opening/En cours d'ouverture, 3=Opened/Ouverte, 4=Closing/En cours de fermeture, 5=Stopped/Arrêtée)
		+ "doorStateText" = door state in text format/État de la porte en format texte.
		+ "alarmState" = alarm state/état de l'alarme(0=None/Pas d'alarme, 1=Chicken detected/Poule détectée, 2=Door blocked opening/Porte bloquée pendant l'ouverture, 3=Door blocked closing/Porte bloquée pendant la fermeture, 4=Opening too long/Ouverture trop longue, 5=Closing too long/Fermeture trop longue, 6=Stopped by user/Stoppé par l'utilisateur).
		+ "alarmStateText" = alarm state in text format/Alarme en format texte.
		+ "sunState" = sun state/état du soleil (0=Unknown/Inconnu, 1=Before open/Avant ouverture, 2=Between open and close/Entre ouverture et fermeture, 3=After close/Après fermeture).
		+ "sunStateText" = sun state in text format/Soleil en format texte.
		+ "manualMode" = manual mode active/Mode manuel actif.
		+ "forcedMode" = forced mode active/Mode forcé actif.
		+ "illumination" = current illumination (0-1204)/Luminosité actuelle.
		+ "motorVoltage" = motor voltage/Tension du moteur.
		+ "motorIntensity" = motor intensity/Intensité du moteur (mA)
		+ "motorDuration" = seconds since motor started/Nombre de secondes depuis le démarrage du moteur.
		+ "lastStatusSent" = seconds since last status sent/Nombre de secondes depuis l'envoi du dernier état.
		+ "chickenDetected" = chicken detected in front of door?/poule détectée devant la porte?
		+ "doorUncertainPosition" = door position uncertain?/Position de la porte incertaine.
		+ "doorOpenPercentage" = door open percentage (valid if doorUncertainPosition is false)/Pourcentage d'ouverture de la porte (valide si doorUncertainPosition est faux).
		+ "now" = current hour and minute/Heure et minute actuels
		+ "sunOpen" = sun door open time/Heure solaire d'ouverture de la porte
		+ "sunClose" = sun door close time/Heure solaire de fermeture de la porte
	- mqttSignalTopic: MQTT signal topic, where information and error messages are sent to (in plain text format)/sujet MQTT où les messages d'information ou d'erreur sont envoyées (en format texte).
	- mqttSettingsTopic: MQTT settings topic, where settings are read (and sent on request). Data is in JSON format. Fields are the same as those in chickenDoorParameters.h, settings part/sujet MQTT où les paramètres sont lus (et écrits sur demande). Les données sont au format JSON. Les champs sont identiques à ceux données dans le fichier chickenDoorParameters.h, partie paramètres.
	- MqttLastWillTopic: MQTT Last Will Topic, where MQTT status is written (contains "up" when module is connected to MQTT, "down" else)/ Sujet MQTT où le statut MQTT est écrit (contient "up" quand le module est connecté à MQTT, "down" sinon).


## Backups/Sauvegardes

As already described, current settings are available in MQTT settings topic through "settings" command. It could be a good idea to save them using any MQTT client software. You may then restore them sending them back to MQTT settings.

Comme déjà dit, les paramètres courants sont disponibles dans le sujet paramètres après une commande "settings".Il peut être habile de les sauvegarder en utilisant n'importe quel client MQTT. Vous pourrez ensuite les restaurer en les renvoyant dans le sujet paramètres.