"""Builds Omniarp-Manual.pdf. Usage: python3 make_manual.py <ui.png> <out.pdf>"""
import sys
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import (BaseDocTemplate, Frame, PageTemplate, Paragraph, Spacer,
                                Table, TableStyle, Image, PageBreak, KeepTogether)

UI, OUT = sys.argv[1], sys.argv[2]
BG, PANEL = colors.HexColor("#241c19"), colors.HexColor("#3a2f2a")
GOLD, RED, TEXT = colors.HexColor("#d9b77a"), colors.HexColor("#ff2a1c"), colors.HexColor("#e9e1d3")

def bg(c, d):
    c.saveState()
    c.setFillColor(BG); c.rect(0, 0, A4[0], A4[1], stroke=0, fill=1)
    c.setFillColor(PANEL); c.rect(0, A4[1] - 14 * mm, A4[0], 14 * mm, stroke=0, fill=1)
    c.setFillColor(GOLD); c.setFont("Helvetica-Bold", 10)
    c.drawString(18 * mm, A4[1] - 9 * mm, "OMNIARP")
    c.setFont("Helvetica", 8)
    c.drawRightString(A4[0] - 18 * mm, A4[1] - 9 * mm, "TICCO ROSS  -  USER MANUAL")
    c.drawCentredString(A4[0] / 2, 10 * mm, str(d.page))
    c.restoreState()

def cover(c, d):
    c.saveState()
    c.setFillColor(BG); c.rect(0, 0, A4[0], A4[1], stroke=0, fill=1)
    c.setFillColor(GOLD); c.setFont("Helvetica-Bold", 58)
    c.drawCentredString(A4[0] / 2, A4[1] - 95 * mm, "OMNIARP")
    c.setFillColor(RED); c.setFont("Helvetica", 15)
    c.drawCentredString(A4[0] / 2, A4[1] - 108 * mm, "ADVANCED  MIDI  ARPEGGIATOR")
    c.setFillColor(GOLD); c.setFont("Helvetica", 11)
    c.drawCentredString(A4[0] / 2, A4[1] - 118 * mm, "USER MANUAL  -  VERSION 1.2.0")
    w = A4[0] - 30 * mm
    c.drawImage(UI, 15 * mm, A4[1] / 2 - 75 * mm, width=w, height=w * 210 / 1525, mask="auto")
    c.setFont("Helvetica-Bold", 12)
    c.drawCentredString(A4[0] / 2, 30 * mm, "TICCO ROSS")
    c.setFont("Helvetica", 10)
    c.drawCentredString(A4[0] / 2, 23 * mm, "ticcoross@gmail.com")
    c.restoreState()

H1 = ParagraphStyle("h1", fontName="Helvetica-Bold", fontSize=18, textColor=GOLD, spaceBefore=10, spaceAfter=8)
H2 = ParagraphStyle("h2", fontName="Helvetica-Bold", fontSize=12.5, textColor=RED, spaceBefore=8, spaceAfter=4)
P = ParagraphStyle("p", fontName="Helvetica", fontSize=10, leading=14.5, textColor=TEXT, spaceAfter=6)
CELL = ParagraphStyle("c", parent=P, spaceAfter=0, fontSize=9.5, leading=13)
CELLB = ParagraphStyle("cb", parent=CELL, fontName="Helvetica-Bold", textColor=GOLD)

def table(rows, widths):
    t = Table([[Paragraph(a, CELLB), Paragraph(b, CELL)] for a, b in rows], colWidths=widths)
    t.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LINEBELOW", (0, 0), (-1, -1), 0.4, colors.HexColor("#5a4a40")),
        ("TOPPADDING", (0, 0), (-1, -1), 5), ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
    ]))
    return t

doc = BaseDocTemplate(OUT, pagesize=A4, leftMargin=18 * mm, rightMargin=18 * mm,
                      topMargin=24 * mm, bottomMargin=20 * mm, title="Omniarp - User Manual", author="Ticco Ross")
fr = Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="f")
doc.addPageTemplates([PageTemplate("cover", [fr], onPage=cover), PageTemplate("body", [fr], onPage=bg)])

from reportlab.platypus import NextPageTemplate
s = [NextPageTemplate("body"), PageBreak()]

s += [Paragraph("Welcome", H1),
      Paragraph("Omniarp is a MIDI arpeggiator that goes further than a classic arp. Hold a chord and it is turned "
                "into a pattern locked to your host's tempo, shaped by eleven algorithms, octave modes, note and octave "
                "repeats, gate and swing.", P),
      Paragraph("Formats: VST3 and AU on macOS (Apple Silicon and Intel), VST3 on Windows, VST3 and LV2 on Linux.", P),
      Paragraph("Installation", H1),
      table([("macOS", "Run <b>Omniarp-Mac.pkg</b>. It installs the AU to /Library/Audio/Plug-Ins/Components and the VST3 "
                       "to /Library/Audio/Plug-Ins/VST3."),
             ("Windows", "Unzip <b>Omniarp-Windows-VST3.zip</b> and copy <b>Omniarp.vst3</b> to "
                         "C:\\Program Files\\Common Files\\VST3."),
             ("Linux", "Extract <b>Omniarp-Linux.tar.gz</b> and copy Omniarp.vst3 to ~/.vst3 and Omniarp.lv2 to ~/.lv2.")],
            [28 * mm, doc.width - 28 * mm]),
      Paragraph("Setting up in your DAW", H1),
      Paragraph("Omniarp produces notes, not sound. It loads as an instrument that sends MIDI out, so you route its notes to the synth you want to play.", P),
      table([("Ableton Live", "Put Omniarp on MIDI track 1 (it appears under Plug-Ins as an instrument; use the VST3). Create MIDI track 2 with your synth, set its MIDI From to track 1 and choose Omniarp, and set Monitor to In. Play or record on track 1."),
             ("Logic Pro", "Load Omniarp as an instrument and route its MIDI output to another instrument track via the track's MIDI input."),
             ("Other hosts", "Insert it on an instrument track before the synth, or route its MIDI output to another track.")],
            [28 * mm, doc.width - 28 * mm]),
      PageBreak()]

s += [Paragraph("The panel", H1), Image(UI, width=doc.width, height=doc.width * 210 / 1525), Spacer(1, 8),
      table([("HOLD", "The blue switch latches the chord. With HOLD on, the arp keeps playing after you release the keys. "
                      "Play a new chord (after letting go of all keys) to replace it; keys added while others are still down join the chord. "
                      "Turning HOLD off releases any latched notes you are no longer holding."),
             ("ALGORITHM", "The red display shows the current pattern. Click it to choose from the list, or scroll over it "
                           "to step through the algorithms."),
             ("SCALE", "The second red display sets a key and scale (Major, Minor, Dorian, Phrygian, Lydian, Mixolydian, Harmonic Minor, "
                       "Pentatonic Major/Minor, Blues). When a scale is on, Omniarp plays every note of that scale from your lowest held note "
                       "up to your highest (at least one octave), so even a single key becomes a melodic run. Click it to choose the key "
                       "(KEY submenu) and scale, or scroll over it. OFF plays just the notes you hold."),
             ("GATE", "Length of each note as a fraction of a step. Low values are short and staccato; 1 is fully legato."),
             ("SWING", "Delays every second step. At 0 the timing is straight; at 1 the off-beat is pushed half a step late."),
             ("NOTE", "Step rate, locked to the host tempo: 1/4, 1/4T, 1/8., 1/8, 1/8T, 1/16., 1/16, 1/16T, 1/32 "
                      "(T = triplet, dot = dotted)."),
             ("NOTE RPT", "Plays each note 1 to 5 times before moving to the next one."),
             ("STEPS", "Pattern length. OFF plays the whole pattern; 1 to 16 loops only the first N steps. Odd lengths "
                       "such as 3, 5 or 7 against a 4/4 beat create shifting polyrhythms."),
             ("OCT", "Octave range: 0 (played octave only), +1, +2, +3 (upwards), -1 (one octave down), "
                     "\u00b11 (one octave below, the played octave, then one above)."),
             ("OCT RPT", "How many times the whole pattern cycles in an octave before moving to the next octave (1 to 5)."),
             ("TRANSPOSE", "Keyboard transpose, made for playing live. Click the red TRANSPOSE display to set the ZONE (OFF, or keys "
                           "below C1, C2, C3 or C4) and the TIMING. Keys below the split no longer join the arp: they transpose it. The C one "
                           "octave below the split means no change, F above it +5, the A flat below it -4, and so on, so you jump straight to "
                           "any interval. The transposition stays after you release the key. TIMING: INSTANT changes on the next step, "
                           "NEXT BEAT or NEXT BAR wait for the beat or bar so changes land on the grid. The display shows the current transposition.")],
            [28 * mm, doc.width - 28 * mm]),
      Paragraph("Click the OMNIARP title to open the About panel; click anywhere on it to close it. Drag the bottom-right corner to resize the window (60% to 200%); the size is saved with your project.", P),
      PageBreak()]

s += [Paragraph("Algorithms", H1),
      Paragraph("Examples use a held chord of C, E, G, B (lowest to highest). Held notes are always sorted by pitch.", P),
      table([("1. UP", "Lowest to highest.  C E G B"),
             ("2. DOWN", "Highest to lowest.  B G E C"),
             ("3. UP/DOWN", "Up then down, without repeating the top and bottom notes.  C E G B G E"),
             ("4. FOLDBACK", "Up then down, repeating the top and bottom notes.  C E G B B G E C"),
             ("5. UP SKIP", "Climbs while jumping ahead two notes.  C G E B G B"),
             ("6. DOWN SKIP", "The mirror of Up Skip, descending.  B E G C E C"),
             ("7. PEDAL UP", "The lowest note alternates with each note above it, going up.  C E C G C B"),
             ("8. PEDAL DOWN", "The highest note alternates with each note below it, going down.  B G B E B C"),
             ("9. PEDAL RAND", "The lowest note alternates with a random note from the rest of the chord.  C ? C ? C ?"),
             ("10. RAND", "Every step is a random note from the chord."),
             ("11. CHORD", "The whole chord plays on every step, which is great for rhythmic stabs with GATE and SWING.")],
            [34 * mm, doc.width - 34 * mm]),
      Paragraph("How the controls combine", H2),
      Paragraph("The final sequence is built in layers: the algorithm makes the basic pattern, NOTE RPT repeats each note "
                "in it, OCT RPT repeats the whole pattern, and OCT moves that through the octave range. For example, UP with "
                "NOTE RPT 2, OCT +1 and OCT RPT 1 on C E G plays: C C E E G G, then C' C' E' E' G' G' an octave higher.", P),
      Paragraph("Timing", H2),
      Paragraph("While the host is playing, Omniarp locks to its timeline, so patterns land on the grid and follow loops "
                "and jumps. When the transport is stopped, it keeps running on its own clock at the host tempo, so you "
                "can play along without pressing play.", P),
      Paragraph("Support", H1),
      Paragraph("Questions, bug reports or ideas: <font color='#d9b77a'>ticcoross@gmail.com</font>", P)]

doc.build(s)
print("ok", OUT)
