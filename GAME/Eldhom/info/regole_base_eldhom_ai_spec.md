# Le Pergamene di Eldhôm — Regole Base AI Spec

Versione: bozza consolidata
Formato: regole schematiche / algoritmi
Scopo: specifica leggibile da AI, software o regolamento tecnico

---

# 0. Convenzioni

## 0.1 Termini principali

```text
PG = Personaggio Giocante
PNG = Personaggio Non Giocante
Mostro = singola miniatura ostile
Gruppo Mostri = insieme di Mostri che agiscono insieme
Boss = Mostro unico con regole dedicate
Locazione = Area tattica della mappa
Prima Linea = posizione tattica avanzata nella Locazione
Retroguardia = posizione tattica arretrata nella Locazione
Formazione = disposizione di una fazione tra Prima Linea e Retroguardia
Schieramento = riorganizzazione volontaria o controllata della Formazione
Scompaginamento = rottura forzata della Formazione
Linea Temporale = tracciato del tempo continuo
⌛ = costo in tempo
❌ = danno
PV = punti vita
```

## 0.2 Icone carta

```text
▶️ = Movimento / posizionamento / spostamento
⏸️ = Attacco / danno / pressione offensiva
⏺️ = Interazione / supporto / scenario / oggetti / recupero
⚡ = Istantanea / reazione fuori turno
```

Le icone indicano la natura dell'effetto, non quale Azione Semplice viene sostituita.

## 0.3 Effetti

### Effetti Negativi

- **Vista Offuscata** : Ai tuoi attacchi di tipo Tiro o Lancio peschi 2 Carte Modificatore e applichi la peggiore. Dura fino alla fine del tuo prossimo turno.

- **Rallentato** : Raddoppia il costo di ogni Azione o Carta che implica un Movimento da una Locazoine ad un'altra. Dura fino alla fine del tuo prossimo turno.

- **Avvelenato** : Applica **Vista Offuscata** e **Rallentato** in modo durarturo fino alla prossima *Cura* ricevuta.

- **Immobilizzato** : La miniatura non può effettuare azioni di Movimento né cambi di Formazione (sia Schieramento, sia Scompaginamento) né essere spostata (salvo specifici teletrasporti). Dura fino alla fine del tuo prossimo turno.

- **Disarmato** : La miniatura non può compiere azioni di attacco. Dura fino alla fine del tuo prossimo turno.

- **Sanguinante** : A inizio di ogni tuo turno, la miniatura subisce 1 danno. Perdura fino alla prossima *Cura* ricevuta.

- **Svenuto**: Impedisce di eseguire qualsiasi azione durante il tuo turno ad eccezione dell'Azione Base **Recupera**. Dura fino alla fine del tuo prossimo turno.

- **Maledetto** : DA DEFINRE

### Effetti Positivi

- **Vista Acuita** : Ai tuoi attacchi di tipo Tiro o Lancio peschi 2 Carte Modificatore e applichi la migliore. Dura fino alla fine del tuo prossimo turno.

- **Concentrato** : riduce di 2⌛ il costo in Tempo della prossima Azione o Carta (minimo 1⌛). Dura per una sola Azione o Carta.

- **Energizzato** : Applica **Vista Acuita** e **Concentrato** in modo durarturo fino alla fine del tuo prossimo turno.

- **Invisibile** : La miniatura non può essere bersaglio di Attacchi. Dura fino alla fine del tuo prossimo turno.

- **Resistente N** : Riduce di N qualunque tipo di Danno ricevuto.Dura fino alla fine del tuo prossimo turno.

- **Benedetto** : DA DEFINRE

---

# 1. Struttura generale del gioco

## 1.1 Nessun Round base

```text
Il gioco NON usa Round nel regolamento base.
Il gioco usa una Linea Temporale continua.
Ogni soggetto ha un segnalino sulla Linea Temporale.
Agisce sempre il soggetto più indietro sulla Linea Temporale.
```

Soggetti sulla Linea Temporale:

```text
- PG
- PNG alleati
- Gruppi Mostri
- Boss / Mostri Unici
- eventuali entità di Missione
```

## 1.2 Regola del tempo

Ogni volta che un soggetto:

```text
- compie una Azione Semplice
- gioca una Carta Azione
- gioca una Carta Istantanea
- risolve un passaggio di Sequenza
- risolve una Carta Comportamento
```

allora:

```pseudocode
soggetto.timeline += costo_⌛
```

Il segnalino avanza subito dopo ogni costo pagato.

Non esiste pagamento cumulativo a fine turno.

---

# 2. Ordine di attivazione

## 2.1 Scelta del prossimo soggetto

```pseudocode
function prossimo_soggetto():
    min_pos = posizione minima sulla Linea Temporale
    candidati = tutti i soggetti con posizione == min_pos
    return risolvi_parità(candidati)
```

## 2.2 Parità sulla Linea Temporale

Priorità standard:

```text
1. PG
2. PNG alleati
3. Gruppi Mostri
4. Boss / Mostri Unici
5. se ulteriore parità: scelgono i Giocatori
```

Missioni, Carte o Schede Mostro possono modificare questa priorità.

---

# 3. Turno del PG

## 3.1 Scelta base

Nel proprio turno, un PG sceglie esattamente una opzione:

```text
A. eseguire 1 Azione Semplice
OPPURE
B. giocare 1 Carta Azione dalla Mano
```

## 3.2 Fine turno PG

Il turno termina quando:

```text
- il PG esegue una Azione Semplice
- il PG gioca una Carta Singola
- il PG gioca una Inizio Sequenza e decide di fermarsi
- il PG gioca una Continuo Sequenza e decide di fermarsi
- il PG gioca una Fine Sequenza
- la Sequenza viene interrotta
- un effetto dichiara la fine del turno
```

Alla fine del turno:

```pseudocode
metti_carte_giocate_negli_scarti()
gestisci_Carta_in_Memoria_se_previsto()
pesca_fino_a_limite_mano()
controlla_prossimo_soggetto_sulla_Linea_Temporale()
```

---

# 4. Scheda minima PG

```text
PG
- Nome
- Livello
- PV attuali
- PV massimi
- Mazzo Totale
- Mazzo Missione
- Mano
- Scarti
- Carte in Memoria
- Limite Mano
- Limite Memoria
- Posizione sulla Linea Temporale
- Locazione attuale
- Posizione interna: Prima Linea / Retroguardia
- Stati / Effetti attivi
- Equipaggiamento
- Affiliazioni
- Etnia
```

Valori base:

```text
PV base: 6
Limite Mano: 5 + Livello
Limite Mazzo Missione: 15 + Livello
Carte in Memoria base: 1
```

---

# 5. Azioni Semplici

Le Azioni Semplici sono sempre disponibili, salvo effetti contrari.

## 5.1 Movimento Semplice

```text
Icona: ▶️
Effetto: muovi fino a 2 Locazioni
Costo: 1⌛
```

Procedura:

```pseudocode
scegli percorso valido fino a 2 Locazioni
per ogni Locazione attraversata:
    verifica collegamento
    applica modificatori di Locazione
    aggiorna posizione
    se entra in nuova Locazione:
        risolvi Schieramento
PG.timeline += 1
turno termina
```

## 5.2 Attacco Semplice

```text
Icona: ⏸️
Effetto: infliggi 1❌ a bersaglio valido
Costo: 2⌛
```

Procedura:

```pseudocode
scegli bersaglio valido
infliggi 1❌
PG.timeline += 2
turno termina
```

## 5.3 Interazione Semplice

```text
Icona: ⏺️
Effetto: interagisci con elemento della scena
Costo: 3⌛
```

Esempi:

```text
- aprire porta
- attivare leva
- raccogliere indizio
- manipolare oggetto
- disinnescare trappola
- usare elemento di mappa
- completare obiettivo
```

Procedura:

```pseudocode
verifica requisito dell'obiettivo
risolvi interazione
PG.timeline += 3
turno termina
```

## 5.4 Recupero Semplice

```text
Icona: ♻️ / ⏺️
Effetto: recupero fisico e gestione Mano
Costo: 3⌛
```

Procedura:

```pseudocode
PG.PV = min(PG.PV + 1, PG.PV_max)
PG può scartare fino a 2 carte
PG pesca stesso numero di carte scartate
PG può ricaricare oggetti/carte con testo "ricaricabile con Recupero"
PG.timeline += 3
turno termina
```

---

# 6. Carte Azione

## 6.1 Classificazione minima carta

Ogni Carta ha almeno:

```text
- Nome
- Origine
- Tipo di uso
- Icone
- Costo in ⌛
- Effetto
- Eventuali Trigger
- Eventuali condizioni
```

## 6.2 Origini carta

```text
- Base
- Etnia
- Affiliazione
- Ruolo / Compagnia
- Ricompensa Missione
- Equipaggiamento
- Arma
- Armatura
- Monile / Anello / Pendaglio
- Artefatto
- Elemento Aeonico / Alchemico
```

## 6.3 Tipi di uso

```text
- Singola
- Inizio Sequenza
- Continuo Sequenza
- Fine Sequenza
- Istantanea
```

---

# 7. Risoluzione Carta Azione

## 7.1 Procedura standard

```pseudocode
function gioca_carta(PG, carta):
    verifica_condizioni(carta)
    applica_effetto(carta)
    PG.timeline += carta.costo_⌛
    se carta non resta in gioco:
        sposta carta negli Scarti
```

## 7.2 Carta Singola

```pseudocode
gioca_carta(PG, carta_singola)
turno termina
```

## 7.3 Carta Inizio Sequenza

```pseudocode
gioca_carta(PG, carta_inizio)
PG può scegliere:
    - fermarsi: turno termina
    - giocare Continuo Sequenza
    - giocare Fine Sequenza
```

## 7.4 Carta Continuo Sequenza

Condizione:

```text
Può essere giocata solo dopo Inizio Sequenza o Continuo Sequenza.
Non può essere prima carta del turno.
```

Procedura:

```pseudocode
gioca_carta(PG, carta_continuo)
PG può scegliere:
    - fermarsi: turno termina
    - giocare altra Continuo Sequenza
    - giocare Fine Sequenza
```

## 7.5 Carta Fine Sequenza

Condizione:

```text
Può essere giocata solo dentro una Sequenza già iniziata.
```

Procedura:

```pseudocode
gioca_carta(PG, carta_fine)
turno termina obbligatoriamente
```

---

# 8. Sequenze

## 8.1 Forma valida

Sequenze valide:

```text
Inizio
Inizio + Fine
Inizio + Continuo
Inizio + Continuo + Fine
Inizio + Continuo + Continuo
Inizio + Continuo + Continuo + Fine
```

Regola:

```text
Una Sequenza deve iniziare con Inizio Sequenza.
Una Sequenza non deve necessariamente terminare con Fine Sequenza.
```

## 8.2 Costo in tempo durante Sequenza

```pseudocode
per ogni carta giocata nella Sequenza:
    risolvi carta
    PG.timeline += carta.costo_⌛
    controlla eventuali Istantanee/Reazioni
```

Il costo non viene pagato alla fine.

## 8.3 Interruzione Sequenza

```pseudocode
se effetto dice "interrompi la Sequenza":
    termina turno del soggetto attivo
    tempo già speso resta valido
    controlla prossimo soggetto sulla Linea Temporale
```

---

# 9. Carte Istantanee

## 9.1 Definizione

Una Carta Istantanea:

```text
- ha icona ⚡
- ha un Trigger
- può essere giocata fuori dal proprio turno
- può essere giocata durante una Sequenza
- costa ⌛
- fa avanzare il cubetto del PG che la gioca
```

## 9.2 Procedura

```pseudocode
quando Trigger si verifica:
    PG può giocare Istantanea valida
    risolvi effetto
    PG.timeline += costo_⌛
    scarta carta salvo diversa indicazione
    se effetto interrompe Sequenza:
        termina turno soggetto attivo
```

---

# 10. Mazzi del PG

## 10.1 Mazzo Totale

```text
Contiene tutte le carte possedute dal PG durante la campagna.
```

## 10.2 Mazzo Missione

```text
Costruito prima della Missione.
Scelto dal Mazzo Totale.
Personale per ogni PG.
Limite: 15 + Livello.
```

## 10.3 Mano

```text
Si pesca dal Mazzo Missione.
Limite: 5 + Livello.
```

## 10.4 Scarti

```text
Carte usate, scartate o risolte finiscono negli Scarti, salvo diversa indicazione.
```

## 10.5 Mazzo Missione esaurito

```pseudocode
se il Mazzo Missione è vuoto e il PG deve pescare:
    rimescola Scarti
    forma nuovo Mazzo Missione
    continua pesca
```

## 10.6 Carte in Memoria

```text
Base: 1 carta.
Effetti, oggetti o affiliazioni possono aumentare il limite.
```

---

# 11. Mappa e Locazioni

## 11.1 Mappa ad Aree / Locazioni

```text
La mappa non usa griglia.
La mappa è composta da Locazioni.
Ogni Locazione può avere forma irregolare.
Ogni Locazione può contenere più miniature.
```

## 11.2 Collegamenti

Due Locazioni sono adiacenti se condividono:

```text
- porta
- passaggio
- apertura
- ponte
- scala
- collegamento esplicito
```

## 11.3 Modificatori di Locazione

Esempi:

```text
- terreno difficile
- passaggio stretto
- altezza
- fuoco
- vapore tossico
- oscurità
- crollo
- porta chiusa
- passaggio segreto
- nessuna Retroguardia
- Retroguardia protetta
- capacità massima
```

---

# 12. Distanza e bersagli

## 12.1 Distanze

```text
Stessa Locazione = mischia
Locazione adiacente = distanza breve
2 Locazioni = distanza media
3+ Locazioni = distanza lunga, solo se permesso
```

## 12.2 Mischia

```text
Un attacco in mischia colpisce bersagli nella stessa Locazione.
```

## 12.3 Distanza

```text
Un attacco a distanza deve indicare portata valida.
```

Esempi:

```text
- bersaglio in Locazione adiacente
- bersaglio entro 2 Locazioni
- bersaglio in linea di vista fino a distanza lunga
```

---

# 13. Ingaggio

## 13.1 Definizione

```text
Un PG è Ingaggiato se nella sua Locazione è presente almeno un Nemico.
Un Mostro è Ingaggiato se nella sua Locazione è presente almeno un PG o Alleato ostile.
```

## 13.2 Effetto base

```text
Essere Ingaggiato NON impedisce automaticamente di muoversi, attaccare, interagire o giocare carte.
```

## 13.3 Trigger possibili

Carte o Mostri possono reagire quando un soggetto Ingaggiato:

```text
- lascia la Locazione
- entra in Retroguardia
- gioca una Carta Sequenza
- gioca una Carta Interazione
- attacca un bersaglio diverso
- usa un oggetto
- attraversa il fronte
- esegue Recupero
```

---

# 14. Prima Linea e Retroguardia

## 14.1 Definizione

Ogni Locazione può contenere miniature in:

```text
- Prima Linea
- Retroguardia
```

Queste posizioni sono tattiche, non caselle fisiche.

## 14.2 Significato

```text
Prima Linea = contatto, pressione, fronte, accesso, corpo a corpo
Retroguardia = supporto, tiro, cura, concentrazione, interazione protetta
```

## 14.3 Regola di Formazione

Per ogni fazione in ogni Locazione:

```text
Retroguardia ≤ Prima Linea
```

Esempi validi:

```text
PL 1 / RG 0
PL 1 / RG 1
PL 2 / RG 1
PL 2 / RG 2
PL 3 / RG 2
```

Esempi non validi:

```text
PL 0 / RG 1
PL 1 / RG 2
PL 2 / RG 3
```

## 14.4 Fazioni separate

La regola si applica separatamente a:

```text
- PG e Alleati PG
- Mostri e Alleati Mostri
- PNG neutrali
- altre fazioni
```

---

# 15. Bersagli con Prima Linea / Retroguardia

## 15.1 Attacco in mischia

```pseudocode
se attaccante è in Prima Linea:
    può colpire nemico in Prima Linea nella stessa Locazione
    può colpire nemico in Retroguardia solo se non ci sono nemici in Prima Linea

se attaccante è in Retroguardia:
    non può attaccare in mischia salvo carta/arma specifica
```

## 15.2 Protezione della Retroguardia

```text
Finché una fazione ha almeno una miniatura in Prima Linea nella Locazione,
la sua Retroguardia non può essere bersagliata in mischia salvo effetti specifici.
```

## 15.3 Attacco a distanza contro Retroguardia

```text
Dalla stessa Locazione: può colpire Retroguardia solo se carta/arma lo permette.
Da altra Locazione: può colpire Retroguardia se linea di vista e mappa lo permettono.
In dubbio: Prima Linea dà copertura alla Retroguardia.
```

---

# 16. Schieramento

## 16.1 Definizione

Schieramento = modifica volontaria o controllata della Formazione.

Rientrano nello Schieramento:

```text
- entrare volontariamente in una Locazione
- uscire volontariamente da una Locazione
- spostarsi volontariamente tra Prima Linea e Retroguardia
- effetto alleato che riorganizza la Formazione
- nuovo Alleato schierato da effetto controllato
```

## 16.2 Proprietà

```text
Schieramento non è una Azione autonoma.
Schieramento non costa ⌛ salvo effetto contrario.
Schieramento avviene come conseguenza di Movimento, Carta o effetto.
Schieramento deve produrre Formazione legale.
```

## 16.3 Schieramento PG

```pseudocode
quando Formazione PG cambia per Schieramento:
    Giocatori interessati decidono di comune accordo nuova Formazione
    nuova Formazione deve rispettare Retroguardia ≤ Prima Linea
    se non c'è accordo:
        decide il Giocatore che controlla la miniatura/effetto che ha causato Schieramento
```

## 16.4 Schieramento Mostri

```pseudocode
quando Mostri entrano o si ridispongono per Schieramento:
    usa regola di Schieramento del Tipo di Mostro
    se assente:
        Mostro con più PV va in Prima Linea
        se parità: Elite prima
        se parità: scelgono i Giocatori
```

Esempi regole Tipo Mostro:

```text
Elite da mischia: Elite prima in Prima Linea.
Elite da distanza: Normali prima in Prima Linea, Elite in Retroguardia.
Branco aggressivo: riempi Prima Linea prima di usare Retroguardia.
Branco guardingo: minimo necessario in Prima Linea, resto in Retroguardia.
```

---

# 17. Scompaginamento

## 17.1 Definizione

Scompaginamento = Formazione rotta da causa esterna o contro volontà della fazione.

Rientrano nello Scompaginamento:

```text
- Spingere
- Tirare
- Spostare forzatamente
- rimuovere miniatura dalla Prima Linea
- uccidere miniatura in Prima Linea
- bandire miniatura in Prima Linea
- Evento che modifica disposizione
- Carta Nemica che altera Formazione
```

## 17.2 Proprietà

```text
Scompaginamento non è una Azione autonoma.
Scompaginamento non costa ⌛.
Scompaginamento non conta come Movimento volontario.
Scompaginamento non attiva effetti "quando ti muovi" salvo specifica.
Scompaginamento può attivare effetti "quando vieni Scompaginato".
```

## 17.3 Scompaginamento PG

```pseudocode
quando Formazione PG viene Scompaginata:
    se Carta/effetto che causa Scompaginamento indica procedura:
        usa quella procedura
    altrimenti:
        usa Indicazioni per lo Scompaginamento
```

## 17.4 Scompaginamento Mostri

```pseudocode
quando Formazione Mostri viene Scompaginata da PG o Alleato PG:
    decide il Giocatore che ha causato lo Scompaginamento
    nuova Formazione deve essere legale
```

## 17.5 Scompaginamento da Evento

```pseudocode
quando Carta Evento causa Scompaginamento:
    se Carta Evento indica procedura:
        usa quella procedura
    altrimenti:
        usa Indicazioni per lo Scompaginamento
```

Se l'Evento colpisce più fazioni:

```text
1. PG e Alleati PG
2. Mostri
3. PNG neutrali
4. altre fazioni
```

---

# 18. Indicazioni per lo Scompaginamento

## 18.1 Procedura

Quando nessuna carta o effetto specifica come risolvere:

```pseudocode
while Formazione illegale:
    candidati = miniature in Retroguardia della fazione
    scegli miniatura che deve passare in Prima Linea usando criteri in ordine
    sposta quella miniatura in Prima Linea
    ricontrolla Formazione
```

## 18.2 Criteri

Passa in Prima Linea:

```text
1. chi ha più PV attuali
2. se parità: chi ha più Carte in Mano
3. se parità: chi è più indietro sulla Linea Temporale
4. se parità: pesca Modificatore più alto
5. se ancora parità: continuare a pescare Modificatori tra i pari
```

## 18.3 Miniature senza Mano

```pseudocode
se miniatura non possiede Mano:
    ignora criterio "più Carte in Mano"
    passa al criterio successivo
```

---

# 19. Chiarimenti Formazione

## 19.1 Passaggio forzato RG -> PL

Il passaggio forzato da Retroguardia a Prima Linea per ristabilire la Formazione:

```text
- non è Azione
- non costa ⌛
- non conta Movimento volontario
- non permette di attraversare Locazioni
- non permette di uscire dalla Locazione
- non evita Ingaggio
- non attiva effetti "quando ti muovi"
- può attivare effetti "quando passi in Prima Linea" o "quando vieni esposto"
```

## 19.2 Formazione illegale

```text
Una Formazione illegale non può essere lasciata irrisolta.
```

Procedura generale dopo ogni effetto:

```pseudocode
applica effetto
per ogni Locazione modificata:
    per ogni fazione presente:
        se Retroguardia > Prima Linea:
            risolvi Schieramento o Scompaginamento
continua gioco
```

---

# 20. Capacità e casi speciali Locazione

## 20.1 Capacità massima

Una Locazione può avere limite massimo.

Esempi:

```text
Passerella: max 2 PL e 1 RG per fazione
Cunicolo: nessuna Retroguardia
Grande Sala: nessun limite
Piattaforma instabile: max 4 miniature totali
```

La regola della Locazione prevale sulla regola generale.

## 20.2 Locazioni senza Retroguardia

```text
In Locazioni senza Retroguardia, tutte le miniature sono in Prima Linea.
```

Esempi:

```text
- ponte stretto
- corridoio angusto
- scala
- passerella sospesa
- tunnel
- zona completamente esposta
```

## 20.3 Retroguardia protetta

Una Locazione può avere Retroguardia protetta.

Esempi:

```text
- barricata
- balcone
- postazione di tiro
- altare rialzato
- console schermata
```

Gli effetti della Retroguardia protetta sono indicati dalla Locazione.

---

# 21. Mostri

## 21.1 Scheda minima Mostro

```text
Mostro
- Nome
- Tipo di Mostro
- Gruppo
- Normale / Elite / Boss
- PV
- Danno base
- Movimento base
- Tratti speciali
- Locazione attuale
- Posizione interna: Prima Linea / Retroguardia
- Stati / Effetti attivi
```

## 21.2 Gruppo Mostri

```text
Gruppo Mostri
- Tipo di Mostro
- Identificatore Gruppo
- Lista miniature
- Segnalino sulla Linea Temporale
- Carta Comportamento attiva
- Mazzo Comportamento del Tipo
- Scarti Comportamento
```

## 21.3 Tipo vs Gruppo

```text
Tipo di Mostro = identità della creatura.
Gruppo Mostri = comportamento tattico locale.
```

Esempio:

```text
5 Goblin normali + 3 Goblin Elite
Gruppo A: 2 Goblin
Gruppo B: 2 Goblin
Gruppo C: 1 Goblin
Elite A: 2 Goblin Elite
Elite B: 1 Goblin Elite
```

Ogni Gruppo ha:

```text
- propria posizione sulla Linea Temporale
- propria Carta Comportamento attiva
```

---

# 22. Carte Comportamento Mostri

## 22.1 Principio

```text
Ogni Tipo di Mostro ha un Mazzo Comportamento.
Ogni Gruppo ha sempre una Carta Comportamento attiva.
La Carta Comportamento indica cosa il Gruppo sta tentando di fare.
La Linea Temporale indica quando il Gruppo agisce.
```

## 22.2 Attivazione Gruppo

```pseudocode
quando Gruppo è più indietro sulla Linea Temporale:
    risolvi Carta Comportamento attiva
    per ogni passaggio risolto:
        Gruppo.timeline += costo_⌛ del passaggio
```

## 22.3 Risoluzione per miniature del Gruppo

```pseudocode
per ogni passaggio della Carta:
    per ogni Mostro del Gruppo:
        se può risolvere passaggio:
            risolve passaggio
        altrimenti:
            salta passaggio
    Gruppo.timeline += costo_⌛ una sola volta
```

Il Gruppo paga tempo una volta per passaggio, non una volta per miniatura.

## 22.4 Se Carta non risolvibile

```pseudocode
se Carta ha comportamento alternativo:
    esegui alternativo
altrimenti:
    esegui Comportamento Base del Tipo
```

---

# 23. Comportamento base Mostri

Quando nessuna istruzione specifica è disponibile:

```pseudocode
if esiste PG bersaglio valido in mischia:
    attacca PG valido
    Gruppo.timeline += 2
else if esiste percorso verso PG più vicino:
    muovi verso PG più vicino fino a Movimento base
    Gruppo.timeline += 1
else:
    resta fermo
    Gruppo.timeline += 1
```

Costi base:

```text
Movimento Mostro: 1⌛
Attacco Mostro: 2⌛
Interazione / capacità Mostro: 3⌛
```

---

# 24. Bersaglio dei Mostri

Se non specificato dalla Carta:

```text
1. PG che ha danneggiato per ultimo un Mostro del Gruppo
2. PG più vicino
3. PG con meno PV
4. PG più indietro sulla Linea Temporale
5. se parità: scelgono i Giocatori
```

Carte Comportamento possono modificare la priorità.

---

# 25. Reazioni dei Mostri

## 25.1 Definizione

Una Carta Comportamento può avere Reazione ⚡.

```text
La Reazione può attivarsi fuori dal turno del Gruppo.
La Reazione ha un Trigger.
La Reazione ha un costo in ⌛.
La Reazione può interrompere Sequenza solo se lo dice.
```

## 25.2 Procedura Reazione Mostro

```pseudocode
quando Trigger si verifica:
    se Carta Comportamento attiva contiene Reazione valida:
        risolvi Reazione
        Gruppo.timeline += costo_⌛
        scarta Carta Comportamento attiva
        pesca nuova Carta Comportamento
        nuova carta diventa attiva subito
        se Reazione interrompe Sequenza:
            termina turno soggetto attivo
```

## 25.3 Reazioni multiple

```text
Un Gruppo può reagire più volte solo se, dopo una Reazione, pesca una nuova Carta Comportamento con nuova Reazione e il Trigger si verifica.
```

Nessun segnalino “Reazione usata” necessario.

---

# 26. Boss

```text
Un Boss è sempre un Gruppo individuale.
Ha proprio segnalino sulla Linea Temporale.
Ha proprio Mazzo Comportamento o set di Carte Comportamento.
Può avere fasi, Reazioni, passive e regole speciali.
```

---

# 27. Eventi di Missione

## 27.1 Eventi temporali

Gli Eventi possono essere legati a soglie sulla Linea Temporale.

Esempi:

```text
Tempo 12⌛: si aprono valvole di vapore
Tempo 24⌛: arrivano rinforzi
Tempo 36⌛: crolla passerella
Tempo 60⌛: Missione fallita
```

Procedura:

```pseudocode
quando tempo_missione raggiunge o supera soglia:
    risolvi Evento associato
```

## 27.2 Eventi non temporali

Possono essere attivati da:

```text
- ingresso in Locazione
- interazione
- morte di un Mostro
- pescata carta
- raggiungimento obiettivo
- scelta narrativa
```

---

# 28. Durata effetti

Non usare “fino alla fine del Round”.

Usare:

```text
- per X⌛
- fino alla tua prossima attivazione
- fino alla prossima attivazione del bersaglio
- finché resti in questa Locazione
- finché resti in Prima Linea
- finché resti in Retroguardia
- finché questa carta resta attiva
- fino al Tempo X della Missione
- fino a quando cambia la Formazione
```

---

# 29. Etnie giocabili

## 29.1 Principio

```text
Etnia = cultura d'origine / abitudine / modo di sopravvivere.
Etnia NON deve sostituire Ruolo, Classe o Affiliazione.
Ogni PG sceglie 1 Etnia alla creazione.
Ogni Etnia fornisce 3 Carte Etnia.
Le Carte Etnia entrano nel Mazzo Totale del PG.
Prima di una Missione, il PG decide se inserirle nel Mazzo Missione entro il limite normale.
```

## 29.2 Etnie attuali

```text
Thael = lavoro duro, attrezzi, pietra, metallo, fatica, competenza materiale
Velyr = grazia sociale, cura leggera, influenza, protezione indiretta
Khar = deserto, sopravvivenza, risorse, memoria, gestione del tempo
Erranti = strade, carovane, debiti, deviazioni, adattamento
```

## 29.3 Non giocabili

```text
Dhôrim = popolo antico, non giocabile
Giganti = rari, PNG
Fauni = rari, PNG
Satiri = rari, PNG
```

---

# 30. Carte Etnia — Thael

## 30.1 Mani da Cava

```text
Origine: Etnia — Thael
Tipo: Singola
Icone: ⏺️
Costo: 2⌛
```

Effetto:

```text
Esegui una Interazione nella tua Locazione.
Se riguarda porte, leve, macchinari, detriti, carichi, serrature grezze, pietra, metallo o strutture danneggiate, scegli 1:
- riduci di 1⌛ il costo aggiuntivo richiesto dall'obiettivo;
- ignora una penalità di Locazione legata a macerie, ostacoli, calore, fumo o terreno difficile;
- pesca 1 carta dopo aver completato l'Interazione.
```

## 30.2 Attrezzo Giusto

```text
Origine: Etnia — Thael
Tipo: Istantanea
Icone: ⚡⏺️
Costo: 1⌛
Trigger: tu o Alleato nella tua Locazione state per eseguire Interazione
```

Effetto:

```text
Scegli 1:
- Interazione costa -1⌛, minimo 1⌛;
- Interazione può essere eseguita anche se il PG è in Prima Linea;
- ignora requisito "non Ingaggiato" per quella Interazione.
```

## 30.3 Schiena Spezzata

```text
Origine: Etnia — Thael
Tipo: Singola
Icone: ⏺️
Costo: 2⌛
```

Effetto:

```text
Recupera 1 PV.
Puoi scartare 1 carta. Se lo fai, scegli 1:
- rimuovi da te uno Stato legato a fatica, veleno, ustione, stordimento o rallentamento;
- ignora fino alla tua prossima attivazione il primo effetto di Terreno Difficile nella tua Locazione;
- pesca 1 carta se sei in Locazione con pietra, metallo, fuoco, fumo, macerie o macchinari.
```

---

# 31. Carte Etnia — Velyr

## 31.1 Parola Giusta

```text
Origine: Etnia — Velyr
Tipo: Singola
Icone: ⏺️
Costo: 2⌛
```

Effetto:

```text
Scegli un Nemico nella tua Locazione o in Locazione adiacente.
Fino alla sua prossima attivazione, la prima volta che quel Nemico dovrebbe usare una Reazione, scegli 1:
- la Reazione costa +1⌛ al suo Gruppo;
- la Reazione non può Scompaginare la Formazione dei PG;
- pesca 1 carta.
```

## 31.2 Loto Bianco

```text
Origine: Etnia — Velyr
Tipo: Istantanea
Icone: ⚡⏺️
Costo: 1⌛
Trigger: Alleato nella tua Locazione subisce danno
```

Effetto:

```text
Riduci quel danno di 1❌.
Se l'Alleato è in Retroguardia, può pescare 1 carta e poi scartare 1 carta.
```

## 31.3 Maschera Gentile

```text
Origine: Etnia — Velyr
Tipo: Istantanea
Icone: ⚡⏺️
Costo: 1⌛
Trigger: Nemico sceglie te come bersaglio di attacco o effetto
```

Effetto:

```text
Se nella tua Locazione c'è almeno un altro PG bersaglio valido, il Nemico deve scegliere bersaglio secondo normali priorità ignorando te.
Se non ci sono altri bersagli validi, riduci di 1❌ il prossimo danno che subisci entro 3⌛.
```

---

# 32. Carte Etnia — Khar

## 32.1 Respiro dell'Oasi

```text
Origine: Etnia — Khar
Tipo: Singola
Icone: ⏺️
Costo: 2⌛
```

Effetto:

```text
Recupera 1 PV.
Guarda le prime 2 carte del Mazzo Missione.
Rimettine 1 sopra e 1 sotto.
Se sei in Retroguardia, scegli anche 1:
- pesca 1 carta e poi scarta 1 carta;
- sposta il tuo cubetto indietro di 1⌛, senza superare il soggetto più indietro sulla Linea Temporale.
```

## 32.2 Acqua Nascosta

```text
Origine: Etnia — Khar
Tipo: Istantanea
Icone: ⚡⏺️
Costo: 1⌛
Trigger: tu o Alleato nella tua Locazione dovreste scartare carta dalla Mano
```

Effetto:

```text
Scegli 1:
- riduci di 1 il numero di carte da scartare;
- dopo aver scartato, quel PG pesca 1 carta;
- quel PG recupera 1 PV se ha 2 PV o meno.
```

## 32.3 Memoria del Sale

```text
Origine: Etnia — Khar
Tipo: Istantanea
Icone: ⚡⏺️
Costo: 1⌛
Trigger: viene rivelata Carta Evento, regola speciale di Locazione o effetto ambientale
```

Effetto:

```text
Dopo aver letto l'effetto, scegli 1:
- guarda la prima carta del Mazzo Missione;
- pesca 1 carta e poi scarta 1 carta;
- ignora per te il primo effetto di quella Carta Evento o Locazione che causerebbe perdita carte, rallentamento o danno ambientale.
```

---

# 33. Carte Etnia — Erranti

## 33.1 Strada di Traverso

```text
Origine: Etnia — Erranti
Tipo: Inizio Sequenza
Icone: ▶️
Costo: 1⌛
```

Effetto:

```text
Muovi fino a 2 Locazioni.
Puoi attraversare una Locazione occupata da Nemici.
Quando entri nella Locazione finale, si risolve normalmente lo Schieramento della Locazione.
Dopo lo Schieramento, se sei in Retroguardia, pesca 1 carta e poi scarta 1 carta.
Puoi continuare la Sequenza.
```

## 33.2 Debito di Carovana

```text
Origine: Etnia — Erranti
Tipo: Singola
Icone: ⏺️
Costo: 2⌛
```

Effetto:

```text
Scegli un Alleato nella tua Locazione o in Locazione adiacente.
Quell'Alleato può recuperare 1 carta non-Istantanea dai propri Scarti e metterla in Mano.
Poi scegli 1:
- scarta 1 carta;
- avanza di +1⌛ aggiuntivo;
- perdi 1 PV.
```

## 33.3 Segno sul Sentiero

```text
Origine: Etnia — Erranti
Tipo: Istantanea
Icone: ⚡⏺️
Costo: 1⌛
Trigger: tu o Alleato nella tua Locazione pescate una o più carte
```

Effetto:

```text
Dopo la pesca, quel PG può mettere 1 carta dalla Mano in cima al proprio Mazzo Missione.
Se lo fa, pesca 1 carta.
```

---

# 34. Affiliazione — Compagnia del Leone

## 34.1 Principio

```text
Affiliazione = addestramento, stile organizzato, ruolo tattico.
Non è Etnia.
```

## 34.2 Ruoli noti

```text
Baluardo = tenere linea, proteggere fronte, assorbire rotture
Stratega = gestire Formazione, coordinare Schieramenti, convertire Scompaginamenti
```

---

# 35. Carte Stratega — Compagnia del Leone

## 35.1 Formazione!

```text
Origine: Affiliazione — Compagnia del Leone
Ruolo: Stratega
Tipo: Singola
Icone: ⏺️
Costo: 1⌛
```

Effetto:

```text
Forza uno Schieramento nella tua Locazione oppure in una Locazione adiacente.
I PG interessati ridispongono la propria Formazione rispettando Retroguardia ≤ Prima Linea.
```

## 35.2 Serrate la Formazione!

```text
Origine: Affiliazione — Compagnia del Leone
Ruolo: Stratega
Tipo: Istantanea
Icone: ⚡⏺️
Costo: 1⌛
Trigger: Formazione dei PG nella tua Locazione viene Scompaginata
```

Effetto:

```text
Converti quello Scompaginamento in uno Schieramento.
I PG interessati decidono di comune accordo nuova Formazione legale.
```

## 35.3 Supporto

```text
Origine: Affiliazione — Compagnia del Leone
Ruolo: Stratega
Tipo: Singola
Icone: ⏺️
Costo: 2⌛
```

Effetto:

```text
Opera uno Schieramento nella tua Locazione.
Se nella Locazione è presente almeno un Baluardo della Compagnia del Leone, scegli 1 PG in Retroguardia.
Quel PG sceglie 1:
- pesca 1 carta;
- applica -1❌ al prossimo danno subito.
L'effetto termina appena cambia la Formazione di quella Locazione.
```

---

# 36. Carte Baluardo — Compagnia del Leone

## 36.1 Pianta di Ferro

```text
Origine: Affiliazione — Compagnia del Leone
Ruolo: Baluardo
Tipo: Istantanea
Icone: ⚡
Costo: 1⌛
Trigger: Formazione dei PG nella tua Locazione viene Scompaginata
```

Effetto:

```text
Se sei in Prima Linea, puoi dichiararti Perno della Formazione.
Durante questo Scompaginamento, se un PG deve passare da Retroguardia a Prima Linea, scegli tu chi passa, ignorando Indicazioni per lo Scompaginamento.
Se scegli te stesso come miniatura che resta esposta o mantiene la linea, riduci di 1❌ il prossimo danno che subisci entro 3⌛.
```

## 36.2 Spalla alla Linea

```text
Origine: Affiliazione — Compagnia del Leone
Ruolo: Baluardo
Tipo: Inizio Sequenza
Icone: ▶️⏺️
Costo: 1⌛
```

Effetto:

```text
Muovi di 1 Locazione.
Quando entri nella nuova Locazione, se ci sono Alleati, puoi Schierarti direttamente in Prima Linea.
Poi i PG interessati possono risolvere uno Schieramento nella Locazione.
Durante questo Schieramento, tu conti come 2 miniature in Prima Linea ai soli fini di Retroguardia ≤ Prima Linea.
Effetto dura fino alla tua prossima attivazione o finché lasci la Locazione.
Puoi continuare la Sequenza.
```

## 36.3 Colpo che Apre

```text
Origine: Affiliazione — Compagnia del Leone
Ruolo: Baluardo
Tipo: Fine Sequenza
Icone: ⏸️
Costo: 2⌛
```

Effetto:

```text
Attacca un Nemico in Prima Linea nella tua Locazione.
Infliggi 2❌.
Se il bersaglio subisce almeno 1❌, puoi Spingerlo in Retroguardia.
Se questo rende illegale la Formazione dei Mostri, si risolve Scompaginamento dei Mostri.
Tu sei il Giocatore che lo ha causato.
Dopo questa carta, la Sequenza termina.
```

---

# 37. Missione

## 37.1 Dati minimi Missione

```text
Missione
- Nome
- Mappa / Locazioni
- Setup PG
- Setup Mostri
- Obiettivi
- Condizioni di Vittoria
- Condizioni di Sconfitta
- Eventi temporali
- Eventi di Locazione
- Regole speciali
- Ricompense
- Conseguenze campagna
```

## 37.2 Condizioni di vittoria

Esempi:

```text
- elimina Boss
- recupera oggetto
- sopravvivi fino a Tempo X
- porta PNG a Locazione X
- attiva meccanismo
- esplora Locazione
- fuggi dalla mappa
```

## 37.3 Condizioni di sconfitta

Esempi:

```text
- tutti i PG sconfitti
- Tempo Missione raggiunge soglia
- obiettivo distrutto
- PNG chiave muore
- Boss fugge
- Evento finale si risolve
```

---

# 38. Stati / Effetti

## 38.1 Struttura minima Stato

```text
Stato
- Nome
- Bersaglio
- Effetto
- Durata
- Trigger di rimozione
- Stackabile sì/no
```

## 38.2 Durata Stato

Usare le durate del capitolo 28.

## 38.3 Esempi Stati

```text
Rallentato: prossimo Movimento costa +1⌛.
Stordito: prossimo Attacco infligge -1❌.
Ustione: subisci 1❌ alla prossima attivazione.
Veleno: quando Recuperi, recuperi 1 PV in meno.
Esposto: prossimo danno subito +1❌.
Protetto: prossimo danno subito -1❌.
```

---

# 39. Algoritmo turno completo PG

```pseudocode
function turno_PG(PG):
    opzione = scegli(Azione_Semplice, Carta_Azione)

    if opzione == Azione_Semplice:
        risolvi_Azione_Semplice(PG)
        fine_turno(PG)
        return

    if opzione == Carta_Azione:
        carta = scegli_carta_dalla_Mano(PG)

        if carta.tipo == Singola:
            gioca_carta(PG, carta)
            fine_turno(PG)
            return

        if carta.tipo == Inizio_Sequenza:
            gioca_carta(PG, carta)
            while PG vuole continuare:
                apri_finestra_Istantanee_Reazioni()
                prossima = scegli Continuo o Fine valida
                gioca_carta(PG, prossima)
                if prossima.tipo == Fine_Sequenza:
                    break
                if Sequenza_interrotta:
                    break
            fine_turno(PG)
            return

        if carta.tipo == Continuo_Sequenza or carta.tipo == Fine_Sequenza:
            errore: carta non giocabile come prima carta
```

---

# 40. Algoritmo attivazione Gruppo Mostri

```pseudocode
function attiva_Gruppo(Gruppo):
    carta = Gruppo.carta_comportamento_attiva

    if carta risolvibile:
        for passaggio in carta.passaggi:
            for mostro in Gruppo.mostri:
                if mostro può risolvere passaggio:
                    risolvi passaggio per mostro
            Gruppo.timeline += passaggio.costo_⌛
            apri_finestra_Istantanee_Reazioni()
            if Sequenza_Gruppo_interrotta:
                break
    else:
        risolvi_comportamento_alternativo_o_base(Gruppo)

    aggiorna_carta_comportamento_se_previsto()
```

---

# 41. Algoritmo controllo Formazione

```pseudocode
function controlla_Formazione(Locazione):
    for fazione in Locazione.fazioni:
        PL = conta_miniature(fazione, Prima_Linea)
        RG = conta_miniature(fazione, Retroguardia)

        if RG <= PL:
            continue

        if causa == volontaria_o_controllata_dalla_fazione:
            risolvi_Schieramento(fazione, Locazione)
        else:
            risolvi_Scompaginamento(fazione, Locazione)
```

---

# 42. Principi di design

```text
Linea Temporale sostituisce Round.
Locazioni sostituiscono griglia.
Prima Linea / Retroguardia sostituiscono micro-posizionamento.
Carte e Sequenze sostituiscono liste fisse di azioni.
Mostri automatici usano Carte Comportamento.
Etnia dà identità culturale.
Affiliazione dà addestramento organizzato.
Ruolo dà funzione tattica.
```

---

# 43. Regole non ancora consolidate

Da definire in futuro:

```text
- Progressione completa PG
- XP / avanzamento livello
- Classi o ruoli base non legati ad Affiliazione
- crafting completo
- economia
- reputazione
- campagna ramificata dettagliata
- città e attività fuori missione
- bilanciamento definitivo carte
- mazzi completi Mostri
- Boss design
- regole avanzate di linea di vista
- regole complete di Equipaggiamento
```

---

# 44. Invarianti principali

```text
1. Non esiste Round base.
2. Agisce chi è più indietro sulla Linea Temporale.
3. Ogni costo in ⌛ avanza subito il segnalino.
4. Il PG fa una Azione Semplice oppure gioca una Carta.
5. Le Sequenze pagano tempo carta per carta.
6. Le Istantanee costano tempo e possono avvenire fuori turno.
7. Ogni Locazione usa Prima Linea e Retroguardia se non specificato diversamente.
8. Per ogni fazione: Retroguardia ≤ Prima Linea.
9. Schieramento = volontario/controllato.
10. Scompaginamento = forzato/contro volontà.
11. Una Formazione illegale va risolta subito.
12. I Mostri agiscono per Gruppi tramite Carte Comportamento.
13. Ogni Gruppo Mostri ha propria Linea Temporale e Carta attiva.
14. Le Reazioni Mostro consumano la Carta Comportamento attiva e ne pescano una nuova.
15. Le durate non usano mai “fine Round”.
```
