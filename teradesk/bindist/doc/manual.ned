Tera Desktop V1.40, 20-08-94, Copyright 1991, 1992, 1993, 1994 W. Klaren.

Inhoud:

1      Inleiding

1.1    De Tera Desktop
1.2    Installatie

2      De menus

2.1    Inleiding
2.2    Desk
2.2.1  Info

2.3    File

2.3.1  Open
2.3.2  Show information
2.3.3  New folder
2.3.4  Close
2.3.5  Close window
2.3.6  Select all
2.3.7  Set filetype
2.3.8  Cycle windows
2.3.9  Quit

2.4    View

2.4.1  Show as
2.4.2  Sort
2.4.3  System en Hidden files

2.5    Options

2.5.1  Install application
2.5.2  Install desk icon
2.5.3  Install window icon
2.5.4  Change icons
2.5.5  Remove icons
2.5.6  Set preferences
2.5.7  Program options
2.5.8  Editor
2.5.9  Window options
2.5.10 Save settings
2.5.11 Load
2.5.12 Save as

3      Iconen en objecten

3.1    Inleiding
3.2    ICONS.RSC
3.3    Het openen van iconen en objecten
3.4    Het kopieren en verschuiven van iconen en objecten
3.5    Selecteren van objecten met behulp van een rubberbox

4      Windows

5      Het ARGV protocol

Dankwoord

1 Inleiding

1.1 De Tera Desktop

De Tera Desktop is een vervanger voor de ST (en TT) desktop. Deze
desktop biedt veel meer mogelijkheden, zoals files en programma's als
icoon op de desktop en files kunnen worden bekeken in een window.
Verder scrollen windows veel sneller dan de windows van andere
desktops.

Dit programma mag vrij gekopieerd worden, onder de voorwaarde dat het
programma in zijn geheel (met volledige documentatie) en onveranderd
verder gekopieerd wordt. Het programma bestaat uit de volgende files:

DESKTOP.PRG
DESKTOP.RSC
DESKFRA.RSC
DESKGER.RSC
ICONS.RSC
README.NED
README.ENG
README.FRA
CHANGES.NED
CHANGES.ENG
CHANGES.FRA
MANUAL.NED
MANUAL.ENG
MANUAL.FRA
CONVERT.PRG
VIEWER.PRG
VIEWER.RSC

Dit programma mag niet zonder schriftelijke toestemming van de auteur
verkocht worden of opgenomen worden in een commercieel programmapakket.
Als het programma wordt opgenomen in PD-bestanden mogen alleen de
kosten van diskettes e.d. doorberekend worden.

Bij het schrijven van de handleiding ben ik ervan uitgegaan dat
iedereen, die dit leest, weet hoe de gewone desktop van de Atari ST
werkt.

1.2 Installatie

De installatie van de Tera Desktop gaat als volgt:

- maak een folder DESKTOP aan op uw harddisk,
- kopieer de files DESKTOP.PRG, DESKTOP.RSC en ICONS.RSC naar deze
  folder.

De desktop kan nu op de gebruikelijke manier gestart worden. Als u
een TOS-versie 1.4 of hoger hebt kunt u er voor zorgen dat na het
booten de desktop automatisch gestart wordt. Dit kan als volgt gedaan
worden:

- Selekteer in de gewone desktop de Tera Desktop (DESKTOP.PRG),
- Klik het menupunt Install application aan,
- Verander de bootstatus van Normal in Auto,
- Klik de button Install aan.

Vergeet hierna niet de desktop te saven.

Als u een computer hebt met TOS 1.0 of TOS 1.2 moet u een speciaal
programma gebruiken om de desktop automatisch te starten. Er zijn
verschillende PD en shareware programma's beschikbaar, die dit
mogelijk maken (niet bij mij verkrijgbaar).

De default naam van de configuratie file is DESKTOP.CFG. Als u wilt
dat afhankelijk van de resolutie een bepaalde desktop configuratie
file wordt geladen moet u file maken met de naam DESKTOP.BAT. Deze
file maakt het ook mogelijk programma's te starten als de desktop
wordt geladen. De file kan er als volgt uit zien:

cd C:\DEMO
DEMO.PRG -a input.dat

#640,200 ST_MED.CFG
#640,400 ST_HIGH.CFG
#640,480 TT_MED.CFG

Met het commando cd kan de huidige directory veranderd worden. Aan
programma's kan een commandoregel meegegeven worden. In de regels die
beginnen met een '#', kan ingesteld worden welke configuratie file
geladen moet worden. Na de '#' volgen de horizontale resolutie,
de verticale resolutie en de naam van de te laden configuratie file.
De horizontale en verticale resolutie moeten van elkaar gescheiden
worden met een komma. Als de huidige resolutie niet in de DESKTOP.BAT
file staat zal de desktop proberen de file DESKTOP.CFG te laden.

Hoewel de desktop ook gebruikt kan worden als u geen harddisk hebt,
wordt het gebruik van een harddisk aangeraden. Het gebruik zonder
harddisk heeft weinig zin, omdat als u bijvoorbeeld een programma op
de desktop zet, dit programma wel steeds beschikbaar moet zijn. Dit
kan natuurlijk opgelost worden door het programma op een ramdisk te
zetten.

De Tera Desktop verbruikt ongeveer 140K geheugen.

De Tera Desktop werkt op alle huidige TOS-versies, maar een
TOS-versie 1.4 of hoger wordt aangeraden, vanwege een aantal bugs in
TOS 1.0 en de een aantal extra mogelijkheden vanaf TOS 1.4. De
desktop werkt niet onder TOS 1.0 op ST-low. Dit wordt veroorzaakt
door een menu dat te groot is.

De belangrijkste redenen om minstens TOS 1.4 te gebruiken zijn:

- De naam van een folder kan veranderd worden,
- Deze versies gaan beter om met files,
- Programma's kunnen na het booten automatisch gestart worden.

De Tera Desktop is getest op TOS 1.0, TOS 1.2, TOS 1.4, KAOS-TOS
1.4.2, TOS 1.6 en TOS 3.05 (TT-TOS). Op deze TOS-versies funktioneerde
de desktop correct, maar bugs zijn niet uit te sluiten. Ook is het
niet uit te sluiten dat het programma verkeerd werkt op andere
configuraties, dan die waarop het programma getest is. Daarom:

De auteur kan niet aansprakelijk gesteld worden voor enige schade
ontstaan door het gebruik van dit programma. Gebruik van het programma
is geheel op eigen risico.

Mocht u een bug vinden, dan stel ik het zeer op prijs als u mij dit
schriftelijk meldt, met een beschrijving van wat er fout gaat en een
beschrijving van uw configuratie (TOS versie, autoboot programma's,
accessoires enz.).

Als u al een oude versie van dit programma hebt (ouder dan 1.20), dan
moet u CONVERT.PRG gebruiken, om de oude configuratie files te
converteren naar een formaat dat de nieuwe desktop begrijpt. Maak
voordat u de configuratie files converteerd een backup van ze, omdat
het conversie programma de files overschrijft met de geconverteerde
files. Het converteren van configuratie files is erg simpel. Start
eerst CONVERT.PRG. Selekteer in de fileselector, die verschijnt, de te
converteren file en klik op OK. Het programma zal de geselekteerde
file converteren. Als de conversie klaar is, dan zal de fileselector
weer verschijnen. Als u het programma wilt verlaten druk dan op
Cancel, selecteer anders de volgende file die geconverteerd moet
worden.

De desktop werkt met enige beperkingen op de resoluties ST-low en
TT-low. De beperkingen zijn dat de dialoogbox voor het invoeren van
de command line van TTP programma's kleiner is en dat de dialoogboxen
van Install Application en Set Preferences niet werken, omdat ze te
groot zijn. Voor het installeren van een applicatie of het veranderen
van de instellingen kunnen de configuratie files voor deze resoluties
ingeladen worden in een hogere resolutie, waarna in deze resolutie
applicaties geinstalleerd kunnen worden en de instellingen veranderd
kunnen worden.

2 De menus.

2.1 Inleiding

In dit hoofdstuk zullen de menus besproken worden. De meeste
menupunten kunnen ook geselekteerd worden met behulp van een toets.
Achter de menupunten is vermeld met welke toets dit gedaan kan
worden. Het teken '^' betekent dat CONTROL tegelijk met de toets erna
ingedrukt moet worden.

Buttons in dialoogboxen kunnen bediend worden met het toetsenbord, 
door de onderstreepte letter in de naam samen met ALTERNATE in te 
drukken. Buttons met een pijltje kunnen bediend worden door CONTROL 
in te drukken met hetzelfde pijltje op het toetsenbord.

2.2 Desk

2.2.1 Info

Met het menupunt Info kan informatie opgeroepen worden over de 
desktop. Naast het copyright staan hierin de hoeveelheid vrij 
geheugen en de TOS-versie van de computer.

2.3 File

2.3.1 Open

Met het menupunt Open kan een geselekteerd object geopend worden. Zie 
voor het openen van objecten ook het hoofdstuk over iconen.

2.3.2 Show information

Met Show information kan informatie opgevraagd worden over 
geselekteerde objecten.

Als het object een file is worden grootte, datum, tijd en naam van de 
file aangegeven. De naam van de file kan eventueel veranderd worden.

In het geval van een folder worden datum, tijd, naam en de ruimte 
die de files in de folder in beslag nemen aangegeven. Vanaf TOS 1.4 
is het mogelijk de naam van de folder te veranderen.

Bij een drive worden de naam van de disk, het aantal files en folders 
op de disk, de ruimte die gebruikt is, de ruimte die nog vrij is en 
de totale ruimte aangegeven. Het is mogelijk dat op een lege drive er 
een verschil van 2K is tussen vrije ruimte en totale ruimte. Dit
wordt veroorzaakt door een bug in de huidige TOS-versies.

Overigens is het mogelijk Show information te gebruiken op meerdere 
objecten. De dialoogbox verschijnt dan opnieuw voor elk geselekteerd 
object.

2.3.3 New folder

Met New folder kan een nieuwe folder aangemaakt worden.

2.3.4 Close

Met Close wordt een window gesloten. In het geval van een directory 
window wordt een niveau in de directory structuur omhoog gegaan, tot 
dat de root van de drive bereikt wordt. Dan wordt het window 
gesloten. Een tekst window wordt meteen gesloten.

2.3.5 Close window

Dit menupunt heeft dezelfde funktie als het menupunt Close. Het 
verschil met Close is dat directory windows meteen gesloten worden, 
ook als het window niet in de root van de drive staat.

2.3.6 Select all

Met dit menupunt kunnen alle objecten in een window geselekteerd
worden.

2.3.7 Set filemask

Met dit menupunt kan ingesteld worden wat voor soort files zichtbaar 
moet zijn in een directory window. U kunt het type intypen, maar u 
kunt ook kiezen uit een aantal voorgedefinieerde types. Als u een van 
deze kiest met een dubbelklik, dan zal de dialoogbox meteen verlaten 
worden en het geselekteerde type gezet worden. Met de buttons Add en 
Delete is het mogelijk zelf types toe te voegen aan en te verwijderen 
uit deze lijst.

2.3.8 Cycle windows

Met dit menupunt wordt het op een na bovenste window het bovenste 
gemaakt. Dit is handig als een window geheel wordt afgedekt door een 
ander window. Het onderste window kan dan boven gehaald worden zonder 
dat het bovenste van grootte of plaats verandert moet worden.

2.3.9 Quit

Met dit menupunt kunt u de desktop verlaten en terugkeren naar de 
gewone desktop.

2.4 View

2.4.1 Show as

Met show as icons en show as text kan ingesteld worden of de files in 
windows te zien zijn als iconen of als tekst. In tekstmode worden naast 
de filenaam ook de lengte, de datum waarop de file gecreerd is en de 
fileattributen getoond. De attributen hebben de volgende betekenis:

    d - directory,
    s - system file,
    h - hidden file,
    w - file is niet tegen schrijven beschermd en
    a - archief bit.

2.4.2 Sort

Met de menupunten Sort by name tot en met Unsorted kan ingesteld 
worden op welke manier de objecten in een directory window gesorteerd 
moeten worden. Als unsorted wordt ingesteld, dan worden de objecten 
gesorteerd in de volgorde waarop ze op de drive staan.

2.4.3 System files en Hidden files

Deze menupunten maken het mogelijk files, waarvan de hidden of system 
vlag gezet is, zichtbaar te maken.

2.5 Options

2.5.1 Install application

Met dit menupunt kan een applikatie geinstalleerd worden. In 
tegenstelling tot de gewone desktop is het mogelijk meerdere filetypes 
op te geven en het formaat van de commandoregel kan opgegeven worden.

Met de buttons Add en Delete kunnen filetypes toegevoegd en verwijderd 
worden. In de commandoregel hebben %f, %n, %F en %N een speciale
betekenis:

%f : Namen van de files waarop gedubbelklikt is of die naar het 
     programma zijn gesleept met volledige padnaam.

%F : Als %f, filenamen worden echter naar kleine letters
     geconverteerd.

%n : Idem, echter zonder volledige padnaam.

%N : Als %n, filenamen worden echter naar kleine letters
     geconverteerd.

Ook kunnen bij commandline eventuele opties van het programma 
opgegeven worden. Zo is het mogelijk LZH files automatisch te laten 
uitpakken als ze geopend worden. Dit gaat als volgt:

- Selekteer LHARC.TTP,
- Kies het menupunt Install application,
- Klik op Add en vul in de dialoogbox die dan verschijnt *.LZH in en 
  klik op OK.
- Typ bij commandline het volgende in:
  e %f
- Klik op Install.

Met de buttons achter application type kan ingesteld worden wat voor 
soort programma het is. Zo is het mogelijk GEM programma's als GTP 
(Gem Takes Parameters) te starten zonder dat de extensie van het 
programma in GTP veranderd hoeft te worden.

Bij Current directory kan ingesteld worden in welke directory het 
programma gestart moet worden. Als Program is geselekteerd wordt het 
programma in zijn eigen directory gestart. Als window geselekteerd is
wordt het programma gestart in de directory van het bovenste directory 
window. Als er geen directory window geopend is wordt de directory van 
het programma genomen.

Met de button Use ARGV protocol kan ingesteld worden of het ARGV 
protokol gebruikt moet worden. Zie voor het ARGV protokol hoofdstuk 5.

De instellingen die hier gemaakt worden, worden ook gebruikt als het 
programma gewoon gestart wordt of als er files naar het programma 
gesleept worden.

Verder is het mogelijk een funktietoets op te geven. Als de ingestelde 
funktietoets wordt ingedrukt zal het programma gestart worden. Voor 
het nummer van de funktietoets kunnen nummers van 1 t/m 20 gebruikt 
worden, waar nummer 1 t/m 10 overeen komen met funktietoets 1 t/m 10 
en waar nummer 11 t/m 20 overeen komen met de SHIFT toets en 
funktietoets 1 tm 10.

Het is niet verplicht om een filetype op te geven, Install Application 
kan ook gebruikt om alleen een funktietoets op te geven of om een van 
de andere opties te veranderen.

Als u met de desktop een programma wist, dat als applikatie 
geinstalleerd is, dan zullen de opties, die met Install Application 
gezet zijn, automatisch gewist worden. Als u buiten de desktop een 
als applikatie geinstalleerd programma wist, dan gebeurt dit niet.
Als de applikatie dan toch gestart wordt, zal een dialoogbox 
verschijnen, waarin gekozen kan worden tussen het wissen van de 
ingestelde opties, het opzoeken van het programma (als het 
bijvoorbeeld verschoven is) en afbreken. Als opzoeken (locate) wordt 
gekozen, dan zal een fileselektor verschijnen, waarmee het programma 
opgezocht kan worden.

2.5.2 Install desk icon

Hiermee kan een icoon op de desktop geinstalleerd worden. Eerst moet 
u met de muis aangeven waar het icoon moet verschijnen. Daarna 
verschijnt een dialoogbox waarin u de naam, eventueel de letter van 
de drive, het soort icoon en de vorm van het icoon kunt instellen. 

Bij het soort icoon kunnen niet alleen drive en trashcan ingesteld 
worden, maar ook printer. Als u files naar een printer icoon sleept, 
dan worden deze afgedrukt op de printer.

Het installeren van files en folders als icoon op de desktop zal 
besproken worden in hoofdstuk 3.

2.5.3 Install window icon

Met dit menupunt kunt u instellen bij welk soort file of folder welk 
icoon hoort. Er zijn twee lijsten met regels hiervoor, een voor files 
en een voor folders. Met de buttons Files en Folders kan tussen beide 
lijsten gekozen worden.

Bij het inlezen van de directory zal voor elke file in de lijsten 
gezocht worden, welk icoon bij deze file hoort. Omdat de regels ook 
wildcards mogen bevatten is de volgorde van de regels erg belangrijk, 
de desktop zal namelijk het icoon nemen dat hoort bij de eerste regel 
in de lijst waaraan een file voldoet.

Met de button Add kan een regel voor iconen worden toegevoegd. De 
nieuwe regel zal op de plaats van de geselekteerde regel tussen 
gevoegd worden.Er zal een dialoogbox verschijnen waarin het filetype 
en het icoon van een regel ingesteld kunnen worden. Als u een regel 
wilt wissen, gebruik dan de button Delete. Met de button Change kan 
een geselekteerde regel veranderd worden.

2.5.4 Change icons

Met Change icon kunnen de vorm en naam van iconen op de desktop 
veranderd worden. Er mogen meerdere iconen tegelijkertijd geselecteerd 
worden.

Van files en folders op de desktop kan alleen het icoontje veranderd 
worden, van drives, printers en trashcans kan ook het type veranderd 
worden.

2.5.5 Remove icons

Met dit menupunt kunnen icoontjes van de desktop verwijderd worden.
Ook hier mogen meerdere iconen tegelijkertijd geselekteerd worden.

2.5.6 Set preferences

Hiermee kunnen een aantal opties voor het kopieren en wissen van 
files, voor dialoog boxen en voor tekst windows ingesteld worden.

Met Confirm copies kan ingesteld worden of er een dialoogbox moet 
verschijnen als files gekopieerd worden. Met Confirm deletes kan 
hetzelfde ingesteld worden voor het wissen van files. Met Confirm 
overwrites kan ingesteld worden of er een dialoogbox moet verschijnen 
wanneer er tijdens het kopieren een file met dezelfde naam wordt 
gevonden of dat files met dezelfde naam meteen overschreven moeten 
worden. Met copybuffer kan de maximale grootte van de buffer, die bij 
het kopieren van files gebruikt wordt, ingesteld worden.

Met tabsize wordt de grootte van de standaard tabulator afstand 
van de tekstwindows ingesteld.

Verder is er een veld, waarin een aantal opties voor dialoogboxen 
ingesteld kan worden. Als eerste kan de mode ingesteld worden. Er zijn 
drie mogelijkheden: normaal, gebufferd en window. Als gebufferd wordt 
gekozen, dan wordt het scherm onder de dialoogbox opgeslagen in een 
buffer. Het voordeel hiervan is dat het scherm onder de dialogbox niet 
opnieuw getekend hoeft te worden na afloop van een dialoog, want het 
wordt teruggekopieerd uit deze buffer. Ook is het in deze mode
mogelijk de dialoogbox te verplaatsen, door deze rechts boven vast te 
pakken met de muis. Als window wordt gekozen voor de mode, dan zal de 
dialoogbox verschijnen in een window. Het voordeel is dat het in deze 
mode mogelijk is de accesories te gebruiken tijdens een dialoog.

Als tweede is het mogelijk in te stellen waar de dialoogboxen moeten 
verschijnen. Er zijn twee mogelijkheden: in het midden van het scherm 
of op de positie van de muis. De laatste mode is vooral handig bij 
beeldschermen met een hoge resolutie.

Met de optie "Save palette" kan worden ingesteld of het kleurenpalet 
moet worden opgeslagen in de configuratie file. Als deze optie 
aanstaat, dan zal het kleurenpalet worden opgeslagen in het geheugen 
als een programma wordt gestart. Na beeindiging van het programma
wordt het kleurenpalet weer teruggezet.

2.5.7 Program options

Hiermee kan ingesteld worden of na TOS programma's op een toets moet 
worden gewacht, welk soort files programma's zijn, wat voor soort 
programma's het zijn en wat de huidige directory moet zijn van de 
programma's bij het starten.

Met de button wait for key kan ingesteld worden, dat de desktop wacht 
tot er een toets of een muisknop wordt ingedrukt na programma's van
het type TOS of TTP.

Met Add kunnen regels voor programmatypes toegevoegd worden. Een 
nieuwe regel wordt toegevoegd op de plaats van de geselekteerde regel. 
Er verschijnt een dialoogbox waarin het soort file, het soort 
programma, de huidige directory en het gebruik van het ARGV protocol 
ingesteld kunnen worden. De volgende soorten programma's zijn 
mogelijk:

    GEM : GEM programma's,
    GTP : GEM takes parameters, GEM programma's waarbij bij het 
          openen een dialoogbox verschijnt waarin een commandoregel 
          kan worden ingetypt.
    TOS : TOS programma's,
    TTP : TOS takes parameters.

In het soort file mogen wildcards gebruikt worden, dus ook hier is de 
volgorde waarin de regels geinstalleerd worden van belang.

Bij de huidige directory kan gekozen worden tussen programma en 
window. Als window is geselekteerd, zal de huidige directory bij het 
starten van het programma gezet worden naar de directory van het 
bovenste window. Als programma geselekteerd is, dan wordt de huidige 
directory gezet naar de directory waar het programma in staat.

Met Change kan de geselekteerde regel veranderd worden en met Delete 
kan de geselekteerde regel gewist worden.

Tijdens het uitvoeren van TOS programma's kan standaard handle 2
omgeleid worden naar het beeldscherm. Deze optie kan ingeschakeld worden
met "use handle 2 as stderr". Deze optie is o.a. handig bij het gebruik
van GNU programma's.

2.5.8 Editor

Met dit menupunt kan de default editor geselekteerd worden. Er zal 
een fileselector verschijnen, waarmee de editor geselekteerd kan 
worden. Als er in een window een programma geselekteerd is, dan zal 
dit programma in de fileselector verschijnen. Anders zal de op dat 
moment geselekteerd editor in de fileselector verschijnen.

2.5.9 Window options

Na het selecteren van dit menupunt verschijnt een dialoogbox, waarmee 
de kleur en het vulpatroon van de desktop achtergrond ingesteld kunnen 
worden. Ook zijn er twee buttons, waarmee het font van de directory en 
viewer windows veranderd kunnen worden. Na het selecteren van een van 
deze buttons verschijnt een dialoogbox, waarin het font kan worden 
geselecteerd. Ook kan de groote van het font ingesteld worden. Door 
de groote te verkleinen passen meer regels in een window. Het is 
alleen mogelijk een ander font te kiezen als GDOS geinstalleerd is. De
font groote kan altijd veranderd worden, ook als GDOS niet 
geinstalleerd is.

2.5.10 Save settings

Hiermee kan de configuratie van de desktop opgeslagen worden.

2.5.11 Load

Hiermee kan een andere configuratie file geladen worden.

2.5.12 Save as

Met dit menupunt kan een configuratie file onder een andere naam 
opgeslagen worden.

3 Iconen en objecten

3.1 Inleiding

In dit hoofdstuk zal het gebruik van iconen besproken worden. Onder 
andere het toevoegen van eigen iconen, het verschuiven en kopieren 
van iconen en het openen van iconen zullen aan de orde komen.

3.2 ICONS.RSC

Deze file bevat de iconen waaruit u kunt kiezen bij Install icon en 
Change icon. Als u een resource editor hebt kunt u zelf uw eigen 
icoontjes toevoegen. Deze icoontjes mogen maximaal 64 pixels breed en 
32 pixels hoog zijn. Voeg geen andere objecten dan icoontjes toe aan 
de resource, omdat anders de desktop ICONS.RSC niet zal inladen. 
Bezitters van een Mega STe of een TT kunnen ook de resource file met 
icoontjes, die bij deze computers wordt meegeleverd, gebruiken. 
Kopieer deze file naar de folder DESKTOP en rename deze naar ICONS.RSC.
De file mag in totaal 64k groot zijn.

Met een resource editor kunnen ook de default icoontjes, die 
bijvoorbeeld verschijnen bij het op de desktop slepen van een file, 
vervangen worden door andere. Dit gaat als volgt:

- Zet de icoontjes die u als default icoontjes wilt gebruiken 
  bovenaan in de box. Hierbij moet de volgende volgorde gebruikt 
  worden:

        1 - disk drive
        2 - hard disk
        3 - file
        4 - programma
        5 - folder
        6 - trash can
        7 - printer

- Sorteer de icoontjes met behulp van de sort funktie van de resource 
  editor,
- Sla de resource file op.

3.3 Het openen van objecten en iconen

Een object in een directory window of een icoontje kan geopend worden 
door erop te dubbelklikken of door het te selekteren en daarna het 
menupunt Open aan te klikken.

Afhankelijk van het object zal het volgende gebeuren:

- disk drive : er wordt een window geopend met daarin de root van 
  deze drive. Disk drives kunnen ook geopend worden door ALTERNATE 
  plus de letter van de drive in te drukken. Wordt naast deze toetsen
  ook de SHIFT toets ingedrukt, dan zal geen nieuw window geopend
  worden, maar dan zal de directory van de drive getoond worden in
  het bovenste al geopende window.
- folder : Als de folder op de desktop staat zal een window geopend 
  worden met daarin de directory van de folder. Als de folder in een 
  window staat zal in dit window de directory van de geopende folder 
  verschijnen. Wordt tijdens het openen ALTERNATE ingedrukt, dan zal 
  een nieuw window geopend worden. De folder '..' gedraagt zich iets 
  anders dan gewone folders. De '..' staat voor een niveau hoger in 
  de directory structuur. Het openen van deze folder heeft hetzelfde 
  effect als het een keer sluiten van een window, echter als 
  ALTERNATE wordt ingedrukt, dan zal een nieuw window verschijnen, 
  met daarin de directory van directory die een niveau boven de 
  huidige ligt.
- file : Als de file een programma is, dan zal dit programma gestart 
  worden. Is het programma van het type GTP of TTP, dan zal eerst nog 
  een dialoogbox verschijnen, waarin de commandoregel van het
  programma ingetypt kan worden. Als de file voldoet aan een van de 
  filetypes geinstalleerd bij Install application, dan zal het 
  bijbehorende programma gestart worden. Wilt u niet dat dit 
  programma gestart wordt, druk dan ALTERNATE in tijdens het openen 
  van de file. De file wordt dan op de gewone manier behandeld. Als de 
  file niet voldoet aan een van deze filetypes, dan zal een dialoogbox 
  verschijnen waarin gekozen kan worden tussen Show, Edit, Print en 
  Cancel. Wordt Show gekozen, dan zal een tekstwindow verschijnen met 
  daarin de file. Als de eerste 256 tekens van de file minder dan 90% 
  ASCII-tekens bevatten, dan zal automatisch de hexmode gekozen 
  worden. Wordt Edit gekozen, dan zal de default editor gestart worden. 
  Als print wordt gekozen dan zal de file afgedrukt worden.

Als een file of folder dat bij een icoontje op de desktop hoort
niet gevonden wordt, dan zal een dialoogbox verschijnen, waarin 
gekozen kan worden tussen opzoeken (locate), icoon wissen (Remove) en 
afbreken. Als opzoeken wordt gekozen, dan zal een dialoogbox 
verschijnen, waarmee de folder of file opgezocht kan worden. 

3.4 Het kopieren en verschuiven van iconen en objecten

Door iconen en objecten te verslepen is het mogelijk om, net als bij 
de gewone desktop, files te kopieren en te wissen. In deze paragraaf 
zal besproken, wat er precies gebeurd als een object ergens naar toe 
gesleept wordt. Afhankelijk van de bestemming gebeurd er het volgende:

- disk drive : het geselekteerde object wordt naar de root van de 
  disk drive gekopieerd. Als het object ook een disk drive is, dan 
  vindt in tegenstelling tot de gewone desktop, een file copy plaats. 
  Wilt u toch een disk direct kopieren naar een andere, dan wordt 
  aangeraden een kopieerprogramma voor diskettes, zoals Fcopy, te 
  gebruiken.

- folder : het geselekteerde object wordt naar deze folder 
  gekopieerd.

- directory window : het object wordt naar de directory gekopieerd, 
  die op dat  moment wordt aangegeven in het window.

- programma : het programma wordt gestart met de objecten als
  parameter.

- desktop : Als het object op de desktop staat, dan zal het object 
  verschoven worden naar de positie van de muis. Is het een object 
  afkomstig uit een directory window, dan zal dit object op de desktop 
  geinstalleerd worden. Het icoontje dat verschijnt, kan op dezelfde 
  manier gebruikt worden als files en folders in een directory window. 
  Er kan slechts een object te gelijk op de desktop gesleept worden.

- trash can : het geselekteerde object wordt gewist.

- printer : het geselekteerde object wordt afgedrukt.

Wordt tijdens het loslaten van objecten boven de bestemming, de
CONTROL toets ingedrukt, dan zal het object niet gekopieerd worden 
naar de bestemming, maar worden verschoven. Was het object een file of 
folder op de desktop, dan zal de directory van het de file of folder 
automatisch aangepast worden.

Als ALTERNATE ingedrukt is bij het loslaten van de muisknop, dan zal 
voordat een file of folder gekopieerd wordt, een dialoogbox 
verschijnen, waarin de file of folder herbenoemd kan worden.

Als een file of folder op de desktop gewist wordt, dan zal ook 
automatisch het bijbehorende icoontje gewist worden.

Als er tijdens het kopieren van files een file of folder wordt 
gevonden met dezelfde naam, dan zal (tenzij u dit hebt uitgeschakeld 
met Set preferences) een dialoogbox verschijnen met daarin de naam 
van de bestaande file en de naam van de file die gekopieerd wordt. Ten 
opzichte van de gewone desktop zijn er de volgende nieuwe 
mogelijkheden:

- Niet alleen de file die gekopieerd wordt, maar ook de bestaande 
  file kan een nieuwe naam krijgen. (Dit werkt alleen als beide files 
  in verschillende directories staan.)
- Wilt u alle bestaande files overschrijven druk dan op de button 
  All. Alle bestaande files zullen dan overschreven worden.

Als tijdens het kopieren er een file en een folder met dezelfde naam
worden gevonden, dan zal de dialoogbox altijd verschijnen, ook als u 
op All hebt gedrukt of als u Confirm overwrites hebt uitgezet.

Als u een folder overschrijft, dan zal de inhoud van deze folder niet 
gewist worden. De inhoud van de folder, die gekopieerd wordt, zal in 
de bestaande gekopieerd worden.

Met Escape kan het kopieren, wissen en afdrukken van files afgebroken 
worden.

3.5 Selecteren van objecten met behulp van een rubberbox

De Tera desktop scrollt windows automatisch als de rubberbox voor 
het selecteren van files buiten het window buiten het window wordt
bewogen. Alle files tussen de regel waar de muis knop is ingedrukt 
en de regel waar de muisknop is losgelaten worden geselekteerd. Let 
erop dat alle files die binnen de rubberbox vallen geselekteerd 
worden, ook als ze buiten het zichtbare deel van het window vallen. 

4 Windows

De desktop kent twee soorten windows, namelijk tekst windows en 
directory windows.In tekst windows worden geopende files weergegeven, 
in directory windows directories van geopende folders en disk drives. 
De meeste funkties zijn al besproken in de vorige hoofdstukken. In 
dit hoofdstuk zullen nog de extra funktietoetsen van de windows 
besproken worden.

Beide soorten windows kunnen met de cursortoetsen bestuurd worden. 
Als tegelijk met de cursortoetsen de SHIFT toets wordt ingedrukt, dan 
wordt een window niet met een regel, maar met een hele pagina 
gescrolld. Met Clr-Home en SHIFT Clr-Home kan naar het begin en het 
einde van een window gesprongen worden.

Een directory window kent de volgende andere toetsen:

Escape : Lees de directory opnieuw in,
C : Heeft dezelfde funktie als CONTROL C en Close window.

Voor tekstwindows kunnen de volgende toetsen gebruikt worden:

Q, C en Escape : sluit window,
Space : volgende pagina,
Return : volgende regel.

Elk tekstwindow heeft een eigen menubalk, waarmee geschakeld kan 
worden tussen de ASCII mode en de hexmode en waarmee de tabulator 
afstand van dat window veranderd kan worden. Als een window geopend 
wordt, dan is de tabulator afstand gelijk aan die ingesteld met 
Set Preferences.

5 Het ARGV protocol

In het GEMDOS is de lengte van commandoregels beperkt tot 125
karakters. Het ARGV protocol maakt veel langere commandoregels 
mogelijk. Een nadeel van deze methode is dat veel programma's het niet 
ondersteunen of er zelfs niet goed mee werken. Daarom is het mogelijk 
bij Install Application en Program options het gebruik van het protocol 
uit te schakelen.

Er zijn twee mogelijkheden om er voor te zorgen dat programma's geen 
last hebben van het ARGV protocol:

- Installeer programma's die last hebben van het ARGV protocol als 
  apart programma type bij Program options (wel voor de types met 
  wildcards installeren) en zet voor dit type het ARGV protocol uit en 
  laat voor de rest het ARGV protocol aan staan.
- Doe het tegenovergestelde: zet voor alle programmatypes het ARGV 
  protocol uit en installeer alle programma's die het ARGV protocol 
  ondersteunen als apart programmatype.

In plaats van een apart programmatype voor een programma, kan een 
programma ook als applicatie geinstalleerd worden, waarbij de opties 
bij Install application gebruikt kunnen worden om het ARGV protocol 
aan of uit te zetten.

Dankwoord

Ik wil graag de volgende personen bedanken voor hun bijdrage aan de Tera 
Desktop:

Ric Klaren,
Ger Hobbelt en
Hans Wessels          voor het maken van een aantal icoontjes, het 
                      testen van de desktop en het verbeteren van een 
                      deel van de programma code.
Keith Frisby en
Mark Matts            voor het verspreiden en ondersteunen van de Tera 
                      Desktop in Groot Brittanie.
Ted Richards          voor het herschrijven van de Engelse handleiding.
Jean Jacques Cortes   voor het vertalen van de resource file in het 
                      Frans.
Joachim Farbowky      voor de Duitse resource file en F_KEYS.RSC
