# H1 – opdateret kildekode

Projektet bygger den nye kildekode til `dist/Release/h1_updated.exe`.
Den oprindelige `../h1cheetos.exe` er ikke blevet overskrevet.
En ZIP med de oprindelige otte kildefiler ligger som `original-<dato>-<tid>.zip`.

## Status

Der er indarbejdet ændringer fra brugerens forumguide og supplerende indlæg.
Bygning og lokale tests kan bekræfte C++-koden og beregningerne, men **ikke** at
de oplyste adresser, pointerkæder, hastigheder eller knogleindeks passer til spillet.
Programmet er endnu ikke valideret mod et kørende H1Z1.

Driverintegration med spillet er ikke udført. Der er nu et separat
[Offline Lab](offline/README.md) med syntetiske data, en afgrænset testdriver og
en selvstændig visning. `kernemul` er stadig et separat emulatorprojekt.

## Start

1. Åbn `dist/Release` og behold de medfølgende DLL-filer sammen med programmet.
2. Start spillet og vent til det er indlæst. Brug vindue eller kantløst vindue til overlayet.
3. Start `h1_updated.exe`. Der er ingen kontrol af et bestemt Windows-brugernavn.
4. Tryk **INSERT** for at åbne den klikbare menu. Fanerne er Visning, Aim og
   Indstillinger. INSERT, Esc eller Luk menu lukker den igen. Musen bruges til
   kontakter og sliders. Aim og legacy-bevægelse er på pause, mens menuen er åben.
5. Se konsollen eller `h1.log` ved fejl. Programmet giver en konkret fejl ved manglende
   spilvindue eller nægtet procesadgang. Administratorrettigheder er ikke en garanti for adgang.
6. F5 slår aim til/fra; hold højre museknap for at aktivere. Det er som standard slået fra.
   Ukendt våben, uenige våbenkæder, fejlede knoglelæsninger eller ukendt mount-status
   forhindrer automatisk sigtning.

Overlay og input er aktive, mens spillet eller menuen har fokus. Ved skift til et
andet program lukkes menuen, og overlayet skjules. End afslutter fra spillet eller menuen.
Den gamle tekstmenu er erstattet af panelet; når det er lukket, vises kun en lille
INSERT/END-hjælpetekst ud over de valgte overlays.

| Tast | Funktion |
|---|---|
| INSERT | Åbn/luk GUI |
| Esc | Luk GUI |
| F1 | Spillervisning |
| F2 | Bokse |
| F3 | Køretøjer |
| F4 | Genstande |
| F5 + hold højre museknap | Aim |
| F6 | Skelet |
| F7 | Linjer |
| End | Afslut |

F11-funktionen magic bullet er fjernet. Den gamle NoRecoil-kontakt havde ingen
tilhørende implementering; F6 bruges nu til skelet. Den tomme F9-køretøjsteleport
er ikke længere vist som en fungerende funktion.

Den eksisterende spillerteleport (F8 + Space) og noclip (F10, S/Z/Q/D,
Space/museknap 4) er bevaret som legacy-funktioner bag `AllowMemoryWrites=1`.
De er som standard deaktiveret og er ikke valideret. De køres højst én gang pr.
billede. Programmet anmoder normalt kun om læseadgang.

## Indstillinger

Ændringer i GUI'en virker med det samme. Tryk **Gem indstillinger** for at bevare
dem ved næste start. Det gælder også ændringer fra F1–F7. Lukning af menuen gemmer
ikke automatisk. **Gendan standardværdier** nulstiller valgene; tryk Gem, hvis de
skal bevares. Gem-funktionen bevarer kommentarer og andre eksisterende INI-nøgler.

Alternativt kan `dist/Release/settings.ini` redigeres manuelt; genstart derefter.

- `FovPixels`: radius omkring skærmens centrum i pixels, ikke grader.
- `Smoothing`: andel af afstanden, som musebevægelsen bruger pr. billede (0,01–1).
- `MaxDistance`: maksimal spillerafstand.
- `DropCompensation=0`: bullet drop er slået fra som standard.
- `Gravity=0`: der blev ikke leveret en bekræftet tyngdekraft for spillet. Når den er
  bestemt, indtastes den her og `DropCompensation` sættes til 1. Testværdien 9,81
  i en unit-test er almindelig eksempeltyngdekraft, ikke en påstand om spillet.
- `Weapon/Path=0`: afprøver begge oplyste kæder. To forskellige genkendte navne
  giver status `Conflicting weapon paths` og deaktiverer aim.
- `Weapon/Path=1`: vælger kun `localPlayer + 0xCE8 -> +0x24 -> tekst`.
- `Weapon/Path=2`: vælger kun `localPlayer + 0x688 -> +0xB8 -> +0x20 -> tekst`.

Kædernes plausible tekst er et tjek, ikke bevis for, at navnet tilhører det aktive våben.
Våbenstatus vises i overlayet. Ukendte våben får ikke en gættet AR-15-hastighed.

Der er ikke implementeret en synlighedstest; forumguidens `bPlayerVisible` er ikke
defineret i de leverede filer. Aim vælger nærmeste gyldige hoved inden for FOV og
kan derfor også vælge en spiller, som er dækket af geometri.

## Byg igen

Forudsætninger: Visual Studio med C++-værktøjer og Windows SDK samt CMake på PATH.
Åbn PowerShell i denne mappe og kør:

```powershell
.\build.ps1
```

Scriptet henter efter behov den fastlåste
[Microsoft.DXSDK.D3DX 9.29.952.8](https://www.nuget.org/packages/Microsoft.DXSDK.D3DX/9.29.952.8),
og [Dear ImGui v1.91.9b](https://github.com/ocornut/imgui/releases/tag/v1.91.9b),
kontrollerer arkivernes SHA-256, bygger x64 Release og kører tests.
DLL-filer og afhængighedernes licenser kopieres med ud. Nye builds overskriver ikke
eksisterende `dist/Release/settings.ini`.

På denne maskine er Visual Studio-projektet `build/H1External.slnx`.

## Hvad blev ændret?

- Klikbar dansk menu med INSERT, sliders, våbenvalg og lagring af indstillinger.
- Menuen tager musefokus, pauser aim/bevægelse og håndterer åbning, lukning og Alt-Tab.
- Windows-brugernavnskontrollen fjernet.
- Hele magic bullet-koden, inputkontakten og menuen fjernet.
- Nye adresser og offsets samlet i `Offs.h`; absolutte adresser bruges uden module-base.
- Opstart giver fejltekst og Windows-fejlkode; debugprivilegiets resultat kontrolleres.
- Kameramatricen læses én gang pr. billede, transponeres og Y-rækken vendes én gang.
- Projektion afviser ugyldige tal, manglende data og punkter bag nærplanet.
- Skelet bruger de leverede indeks, herunder højre ben 110/111/113, og yaw-rotation.
- Køretøjsposition og -hastighed anvendes, når mount-kæden kan læses.
  Mislykket health-læsning vises ikke som et sikkert dødsfald, og køretøjs-HP
  vises som ukendt, fordi den kan være misvisende.
- Aim vælger ét mål og foretager højst én museopdatering pr. billede.
- Hastighedsforudsigelse og valgfri drop-kompensation sker i verdenskoordinater før W2S.
- Hukommelseslæsninger, navnestrenge og entity-løkker er begrænsede og kontrolleres.
- Overlayet følger spillets klientområde og håndterer ændret størrelse/tabt grafikdevice.
- Den gamle `Math.h` er bevaret som `LegacyMath.h`, så den ikke skygger for Windows'
  standardheader `math.h`. Den aktive matematik ligger i `RuntimeLogic.h`.

## Validering

Seneste kontrol (2026-09-28): x64 Release bygget; logik-, hukommelses- og
GUI-testgrupper bestået. `--check-startup` bestået. GUI'en er renderet og visuelt
kontrolleret i et skjult testvindue. Klik, sliders, faner, gem/indlæs og ændret
vinduesstørrelse er afprøvet med syntetiske input; ingen input er sendt til spillet.
INSERT/fokusskift og overlay er endnu ikke afprøvet i en live spilsession.

`build.ps1` kører tests for projektion, matrixretning, skeletrotation, tyngdekraft,
våbengenkendelse, konflikt mellem våbenkæder, health-kontrol, målvalg og fejlede
hukommelseslæsninger. Memory-testen læser kun sin egen testproces.

```powershell
.\dist\Release\h1_updated.exe --check-startup
```

Denne kontrol læser indstillinger og kontrollerer opstart uden at tilslutte et spil
eller sende museinput. `--diagnose` forsøger derimod at finde spillet og læse kameraet,
men åbner ikke et overlay og sender ikke museinput.

Se `docs/FORUM_NOTES.md` for alle supplerende oplysninger og deres usikkerheder,
og `docs/original-guide.txt` for den fulde guide, som brugeren vedhæftede.
