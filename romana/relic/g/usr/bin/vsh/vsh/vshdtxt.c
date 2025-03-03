/*
 *      vsh: text module German
 *
 *      Copyright (c) 1990-93 by Udo Munk
 */

#ifdef AIX
#define NLS
#endif

#include <curses.h>
#include "vsh.h"
#include "winfun.h"

char yestxt[] = "Ja";
char notxt[] = "Nein";
char oktxt[] = "Ok";

char *l0txt[] = {
	"System:",
	"Datum:",
	"Zeit:",
	"Leitung:",
	"Login:",
	"UID:",
	"GID:",
	"Dateien:      ",
	"Groesse Dat.: ",
	"Dateien mark.:",
	"Groesse m. D.:",
	"Ver. Stack:   ",
	"E-Mail:       "
};

#ifdef SHAREWARE
char *share_box[] = {
	"Dies ist eine unregistrierte Shareware Version.",
	"          Bitte registrieren lassen.",
	NULL,
	oktxt,
	NULL
};
#endif

char *copyright_box[] = {
#ifdef COHERENT
	"             Visual Shell for COHERENT",
	"                   Release 2.9-D",
	"         Copyright (c) 1990-93 by Udo Munk",
	"",
	"Dieses Programm ist lizensiert fuer Mark Williams Co.",
	"und gehoert zum Lieferumfang dieses COHERENT Releases",
#else
#ifdef AIX
	"       Visual Shell for AIX",
	"           Release 2.9-D",
	"Copyright (c) 1990-93 by Udo Munk",
#else
	"    Visual Shell for System V",
	"          Release 2.9-D",
	"Copyright (c) 1990-93 by Udo Munk",
#endif
#endif
	NULL,
	oktxt,
	NULL
};

char *sand[] = {
	" \\---/ ",
	"  \\_/  ",
	"  / \\  ",
	" /---\\ "
};

char *l0mail[] = {
	"Keine",
	"Vorh.",
	"Neue"
};

char *l0msg[] = {
	"Nachrichten moeglich    ",
	"Nachrichten nicht moegl."
};

char *l0menu[] = {
	"Datei   ",
	"Verzei. ",
	"Option  ",
	"Install ",
	"Befehl  ",
	"Refresh ",
	"Ende    ",
	"Hilfe   "
};

char *exit_box[] = {
	"Wollen Sie das Programm wirklich beenden ?",
	NULL,
	yestxt,
	notxt,
	NULL
};

char *restrict_box[] = {
	"Sie arbeiten mit einer eingeschraenkten vsh !",
	"Fragen Sie den System Administrator !",
	NULL,
	oktxt,
	NULL
};

char *msgs[] = {
	"Bitte eine Taste betaetigen...",
	"Das Programm kann nicht mit Ein-/Ausgabeumlenkung laufen",
	"Das Terminal muss mindestens 23 Zeilen und 80 Spalten haben",
	"Das Programm kann nicht auf die Lock-Datei zugreifen",
	"Die maximal zulaessige Anzahl vsh's laeuft bereits",
	"SHELL: Durch Eingabe von <cntl-d> oder 'exit' zurueck zur vsh",
	"Der Befehl %s kann nicht ausgefuehrt werden\n"
};	

char *options[] = {
	"Unbekannte Option %c; bekannte Optionen sind:\n",
	"-d [directory] Start vom momentanen oder dem angegebenen Directory\n",
	"-r             eingeschraenkte Version\n",
	"-i             Installation nicht erlauben\n",
	"-t             VT100 Graphik-Zeichen verwenden\n",
	"-e             keine Graphik-Zeichen verwenden\n",
	NULL
};

struct menu f_pulldown[] = {
	{ "Datei", 0 },
	{ "Kopieren", 1 },
	{ "Verschieben", 1 },
	{ "Loeschen", 1 },
	{ "Umbenennen", 1 },
	{ "Ausfuehren", 4 },
	{ "Zugriffsrechte", 1 },
	{ "Eigentuemer", 3 },
	{ "Drucken", 1 },
	{ "Anzeigen", 1 },
	{ "Editieren", 1 },
	{ "Neue Datei ed.", 1 },
	{ "Zeitstempel", 4 },
	{ "Markieren ein", 1 },
	{ "Markieren aus", 3 },
	{ "Selektieren", 1 },
	{ "Datei Type", 8 },
	{ "Datei Info", 7 },
	{ NULL, 0 }
};

struct menu d_pulldown[] = {
	{ "Verzeichnis", 0 },
	{ "Wechseln", 1 },
	{ "Home", 1 },
	{ "Anwender Home", 1 },
	{ "Push", 1 },
	{ "Pop & wechseln", 2 },
	{ "Umschalten", 8 },
	{ "Kopieren", 1 },
	{ "Loeschen", 1 },
	{ "Umbenennen", 1 },
	{ "Neu anlegen", 1 },
	{ "Zugriffsrechte", 1 },
	{ "Eigentuemer", 3 },
	{ "Neu einlesen", 5 },
	{ "Info", 1 },
	{ NULL, 0 }
};

struct menu c_pulldown[] = {
	{ "Befehl", 0 },
	{ "Befehl", 1 },
	{ NULL, 0 }
};

struct menu r_pulldown[] = {
	{ "Refresh", 0 },
	{ "Refresh", 1 },
	{ NULL, 0 }
};

struct menu e_pulldown[] = {
	{ "Ende", 0 },
	{ "Ende", 1 },
	{ NULL, 0 }
};

struct menu h_pulldown[] = {
	{ "Hilfe", 0 },
	{ "Info", 1 },
	{ NULL, 0 }
};

struct menu o_pulldown[] = {
	{ "Optionen", 0 },
	{ "Shell", 1 },
	{ "Terminal abschliessen", 1 },
	{ "Nachrichten", 1 },
	{ "Handbuch", 1 },
	{ NULL, 0 }
};

struct menu i_pulldown[] = {
	{ "Install", 0 },
	{ "Bildschirm", 1 },
	{ "Bildschirm-Schoner", 2 },
	{ "Funktionstasten", 1 },
	{ "Shell", 1 },
	{ "Editor", 1 },
	{ "Druck Befehl", 1 },
	{ "Dateianzeige Befehl", 2 },
	{ "Datei Aktionen", 8 },
	{ NULL, 0 }
};

struct menu id_pulldown[] = {
	{ "Bildschirm Attribute", 0 },
	{ "Menueleiste", 6 },
	{ "Menue Farbe", 7 },
	{ "Menue Attribut", 7 },
	{ "Dialogboxen", 1 },
	{ NULL, 0 }
};

struct menu ik_pulldown[] = {
	{ "Funktionstasten", 0 },
	{ "Funktionstaste 1", 16 },
	{ "Funktionstaste 2", 16 },
	{ "Funktionstaste 3", 16 },
	{ "Funktionstaste 4", 16 },
	{ "Funktionstaste 5", 16 },
	{ "Funktionstaste 6", 16 },
	{ "Funktionstaste 7", 16 },
	{ "Funktionstaste 8", 16 },
	{ "Funktionstaste 9", 16 },
	{ NULL, 0 }
};

struct menu ia_pulldown[] = {
	{ "Datei Aktionen", 0 },
	{ "Aktionsliste editieren", 14 },
	{ "Aktion konfigurieren", 10 },
	{ NULL, 0 }
};

char r_txt[] = "Lesen";
char w_txt[] = "Schreiben";
char x_txt[] = "Ausfuehren";
char *acc_text1 = "Zugriffsrechte der Datei %s aendern";
char *acc_text2 = "Zugriffsrechte aller markierten Dateien aendern";

struct button acc_ow_r = {
	r_txt,
	0
};

struct button acc_ow_w = {
	w_txt,
	0
};

struct button acc_ow_x = {
	x_txt,
	0
};

struct button acc_gr_r = {
	r_txt,
	0
};

struct button acc_gr_w = {
	w_txt,
	0
};

struct button acc_gr_x = {
	x_txt,
	0
};

struct button acc_wo_r = {
	r_txt,
	0
};

struct button acc_wo_w = {
	w_txt,
	0
};

struct button acc_wo_x = {
	x_txt,
	0
};

struct button acc_su = {
	"UID setzen",
	0
};

struct button acc_sg = {
	"GID setzen",
	0
};

struct button acc_st = {
	"Sticky",
	0
};

struct bgroup acc_g_owner = {
	"Eigentuemer",
	3,
	{ &acc_ow_r, &acc_ow_w, &acc_ow_x }
};

struct bgroup acc_g_group = {
	"Gruppe",
	3,
	{ &acc_gr_r, &acc_gr_w, &acc_gr_x }
};

struct bgroup acc_g_world = {
	"Alle",
	3,
	{ &acc_wo_r, &acc_wo_w, &acc_wo_x }
};

struct bgroup acc_g_special = {
	"Spezial",
	3,
	{ &acc_su, &acc_sg, &acc_st }
};

struct bbox access_buttons = {
	NULL,
	4,
	{ &acc_g_owner, &acc_g_group, &acc_g_world, &acc_g_special }
};

char *own_text1 = "Eigentuemer/Gruppe der Datei %s aendern";
char *own_text2 = "Eigentuemer/Gruppe aller markierten Dateien aendern";

struct delem own_e_uid = {
	FIELD,
	"Neuer Eigentuemer:",
	8
};

struct delem own_e_gid = {
	FIELD,
	"Neue Gruppe:      ",
	8
};

struct dial own_dialog = {
	NULL,
	2,
	{ &own_e_uid, &own_e_gid }
};

char *own_err1 = "Der eingegebene %sname ist unbekannt";
char *own_uname = "Benutzer";
char *own_gname = "Gruppen";
char *f_own_box[] = {
	NULL,
	NULL,
	oktxt,
	NULL
};

char *f_print_box[] = {
	"Moechten Sie wirklich drucken ?",
	NULL,
	yestxt,
	notxt,
	NULL
};

char *f_rm_text = "Es sind %d Dateien markiert";
char *f_rm_box[] = {
	NULL,
	"Was moechten Sie loeschen:",
	NULL,
	"Keine",
	"Alle",
	"Einzeln",
	NULL
};

char *f_rmi_text = "Datei %s loeschen ?";
char *f_rmi_box[] = {
	NULL,
	NULL,
	yestxt,
	notxt,
	NULL
};

char *f_re_box[] = {
	"Neuen Dateinamen eingeben",
	NULL
};

char *f_re1_box[] = {
	"Umbenennen nicht moeglich, Datei existiert bereits",
	NULL,
	oktxt,
	NULL
};

char *f_re2_box[] = {
	"Umbenennen nicht moeglich, Datei ist schreibgeschuetzt",
	NULL,
	oktxt,
	NULL
};

struct delem cp_e_fn = {
	FIELD,
	"Dateiname:"
};

struct delem cp_e_pn = {
	FIELD,
	"Pfadname: "
};

struct dial cp_dialog = {
	"Ziel-Dateiname und/oder Pfadname eingeben",
	2,
	{ &cp_e_fn, &cp_e_pn }
};

char *f_cp1_box[] = {
	"Zielverzeichnis eingeben",
	NULL
};

char *f_cp2_txt = "Datei %s existiert, ueberschreiben ?";
char *f_cp2_box[] = {
	NULL,
	NULL,
	yestxt,
	notxt,
	NULL
};

char *f_cp3_box[] = {
	"Kopieren nicht moeglich, Zielverzeichnis ist schreibgeschuetzt",
	NULL,
	oktxt,
	NULL
};

char *f_show_box[] = {
	NULL,
	NULL,
	oktxt,
	NULL
};

char *finfo_txt[] = {
	"Dateiname      ",
	"Dateitype      ",
	"I-Node         ",
	"Links          ",
	"Eigentuemer UID",
	"Eigentuemer GID",
	"Zugriff        ",
	"Modifikation   ",
	"Statusaenderung"
};

char *ftype_txt[] = {
	"FIFO",
	"Verzeichnis",
	"Zeichen-Geraetetreiber",
	"Block-Geraetetreiber",
	"Datei",
	"unbekannt"
};

char *f_info_box[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	oktxt,
	NULL
};

char *f_select_box[] = {
	"Muster zur Selektion von Dateien eingeben:",
	"[c1c2...cn-cm] Zeichenklasse",
	"?              beliebiges Zeichen",
	"*              beliebige Anzahl beliebiger Zeichen",
	NULL
};

char *f_exec_box[] = {
	"Parameter eingeben",
	NULL
};

char *d_not_box[] = {
	"Befehl mit dem Verzeichnis .. nicht moeglich !",
	NULL,
	oktxt,
	NULL
};

char *d_nf_box[] = {
	"Der Befehl kann nur auf Verzeichnisse angewendet werden !",
	NULL,
	oktxt,
	NULL
};

char *f_nd_box[] = {
	"Der Befehl kann nur auf Dateien angewendet werden !",
	NULL,
	oktxt,
	NULL
};

char *d_cdu_box[] = {
	"Login id des Anwenders eingeben",
	NULL
};

char *d_nous_box[] = {
	"Anwender nicht gefunden",
	NULL,
	oktxt,
	NULL
};

char *d_ncd_box[] = {
	"In das Verzeichnis kann nicht gewechselt werden",
	NULL,
	oktxt,
	NULL
};

char *d_nopath_box[] = {
	"Das eingegebene Verzeichnis existiert nicht",
	NULL,
	oktxt,
	NULL
};

char *d_nrd_box[] = {
	"Das Verzeichnis kann nicht gelesen werden",
	NULL,
	oktxt,
	NULL
};

char *d_cre_box[] = {
	"Neuen Verzeichnisnamen eingeben",
	NULL
};

char *d_cre1_box[] = {
	"Einrichten nicht moeglich, Verzeichnis ist schreibgeschuetzt",
	NULL,
	oktxt,
	NULL
};

char *d_cre2_box[] = {
	"Einrichten nicht moeglich, Verzeichnis existiert bereits",
	NULL,
	oktxt,
	NULL
};

char *d_rm_text = "Verzeichnis enthaelt %d Dateien und %d Verzeichnisse";
char *d_rm_box[] = {
	NULL,
	"Wollen Sie dieses Verzeichnis wirklich loeschen ?",
	NULL,
	yestxt,
	notxt,
	NULL
};

char *d_push_box[] = {
	"Verzeichnis-Stack ist voll",
	NULL,
	oktxt,
	NULL
};

char *d_pop_box[] = {
	"Verzeichnis-Stack ist leer",
	NULL,
	oktxt,
	NULL
};

char *lock1_box[] = {
	"Passwort eingeben",
	NULL
};

char *lock2_box[] = {
	"Diese Terminal ist abgeschlossen!",
	" ",
	"Zum Aufschliessen Passwort eingeben",
	"oder Return-Taste zum Ausloggen betaetigen",
	NULL
};

struct delem top_delm = {
	FIELD,
	"Begriff:",
	30
};

struct delem cha_delm = {
	FIELD,
	"Kapitel:",
	5
};

struct dial man_dialog = {
	"Einen Begriff eingeben, Kapitel ist optional:",
	2,
	{ &top_delm, &cha_delm }
};

char *ins_shell_box[] = {
	"Befehl eingeben, um eine Shell zu starten",
	"(Standard ist '/bin/sh')",
	NULL
};

char *ins_editor_box[] = {
	"Befehl eingeben, um einen Editor zu starten",
	"(Standard ist 'vi')",
	NULL
};

char *ins_print_box[] = {
	"Befehl eingeben, um eine Datei zu drucken",
#ifndef COHERENT
	"(Standard ist 'lp')",
#else
	"(Standard ist 'lpr -B')",
#endif
	NULL
};

char *ins_view_box[] = {
	"Befehl eingeben, um eine Datei anzuzeigen",
#ifndef COHERENT
	"(Standard ist 'pg')",
#else
	"(Standard ist 'more')",
#endif
	NULL
};

char *ins_sav_box[] = {
	"Zeit in Minuten fuer den Bildschirmschoner eingeben",
	NULL
};

char *ins_menu_box[] = {
	"Waehlen Sie ein Attribut fuer die Menueleiste",
	NULL,
	"Fett",
	"Unterstreichen",
	"Invertiert",
	NULL
};

char *ins_pdco_box[] = {
	"Waehlen Sie die Farbe der Menues",
	NULL,
	"Normal",
	"Invertiert",
	NULL
};

char *ins_puld_box[] = {
	"Waehlen Sie ein Attribut fuer die Menues",
	NULL,
	"Fett",
	"Unterstreichen",
	"Beides",
	"Normal",
	NULL
};

char *ins_dial_box[] = {
	"Waehlen Sie ein Attribut fuer die Dialogboxen",
	NULL,
	"Fett",
	"Unterstreichen",
	"Beides",
	NULL
};

struct delem pfkey_label = {
	FIELD,
	"Label :",
	PFKLABLEN
};

struct delem pfkey_cmd = {
	FIELD,
	"Befehl:",
	60
};

struct dial pfk_dialog = {
	"Konfiguration fuer die Funktiontaste eingeben",
	2,
	{ &pfkey_label, &pfkey_cmd }
};

char *set_msg_box[] = {
	"Moechten Sie Nachrichten empfangen ?",
	NULL,
	yestxt,
	notxt,
	NULL
};

char *f_err1 = "Datei %s kann nicht %s werden";
char *f_err2 = "verschoben";
char *f_err3 = "geloescht";
char *f_err4 = "kopiert";
char *f_err5 = "umbenannt";
char *f_err6 = "geaendert";
char *f_err7 = "angelegt";

char *sf_err_box[] = {
	NULL,
	NULL,
	oktxt,
	NULL
};

char *tf_err_box[] = {
	NULL,
	NULL,
	"abbrechen",
	"weiter",
	NULL
};

char *ins_act_box[] = {
	"Sollen die Datei Aktionen vor der Ausfuehrung angezeigt werden ?",
	NULL,
	yestxt,
	notxt,
	NULL
};

LISTBOX fa_list_box = {
	"Editieren der Datei-Aktionen Liste",
	10,
	MAXACTSIZE,
	1
};
