/*
 * File:   servo-pwm-2.c
 * Author: jmgonet
 *
 * Created on March 10, 2014, 7:37 PM
 */

#include <xc.h>

/**
 * Bits de configuration:
 */
#pragma config FOSC = INTIO67  // Osc. interne, A6 et A7 comme IO.
#pragma config IESO = OFF      // Pas d'osc. au démarrage.
#pragma config FCMEN = OFF     // Pas de monitorage de l'oscillateur.

// Nécessaires pour ICSP / ICD:
#pragma config MCLRE = EXTMCLR // RE3 est actif comme master reset.
#pragma config WDTEN = OFF     // Watchdog inactif.
#pragma config LVP = OFF       // Single Supply Enable bits off.

signed char position = 0;

/**
 * Gère la séquence PWM pour le servomoteur.
 */
void PWM_sequence() {
    static char n = 0;

    // Met à jour le rapport cyclique:
    switch(n) {
        case 0:
            CCPR5L = 94 + position;
            break;

        default:
            CCPR5L = 0;
    }

    // Suivante étape dans la séquence
    // du rapport cyclique.
    n++;
    if (n > 9) {
        n = 0;
    }
}

/**
 * États pour la machine à états.
 */
enum Etats {
    ARRET,
    BALAYE1,
    BALAYE
};

/**
 * État actuel de la machine à états.
 */
enum Etats etat = ARRET;

/**
 * Événements pour la machine à états.
 */
enum Evenements {
    EINT0,
    EINT1,
    TICTAC
};

/**
 * Machine à états.
 * @param e événement à traiter.
 */
void SERVO_machine(enum Evenements e) {
    static unsigned char n;
    static unsigned char vitesse;
    static signed char sens = 1;

    switch(etat) {
        // L'essuie-glaces est à l'arrêt:
        case ARRET:
            switch(e) {
                case EINT0:
                    vitesse = 10;
                    sens = 1;
                    etat = BALAYE1;
                    break;

                case EINT1:
                    vitesse = 10;
                    sens = 1;
                    etat = BALAYE;
                    break;
            }
            break;

        // Effectue 1 balayage, puis s'arrête:
        case BALAYE1:
            switch(e) {
                case TICTAC:
                    n++;
                    if (n >= vitesse) {
                        n = 0;

                        position += sens;

                        if (position > 31) {
                            position = 31;
                            sens = -1;
                        }
                        if (position < -31) {
                            position = -31;
                            etat = ARRET;
                        }
                    }
                    break;
            }
            break;

        // Balaye en continu:
        case BALAYE:
            switch(e) {
                case EINT0:
                    etat = BALAYE1;
                    break;

                case EINT1:
                    if (vitesse == 10) {
                        vitesse = 5;
                    } else {
                        vitesse = 10;
                    }
                    break;

                case TICTAC:
                    n++;
                    if (n >= vitesse) {
                        n = 0;

                        position += sens;

                        if (position > 31) {
                            position = 31;
                            sens = -1;
                        }
                        if (position < -31) {
                            position = -31;
                            sens = 1;
                        }
                    }
                    break;
            }
            break;
    }
}

/**
 * Gère les interruptions de haute priorité.
 */
void interrupt interruptionsHP() {
    // Est-ce bien une interruption de INT0?
    if (INTCONbits.INT0IF) {
        // Abaisse le drapeau:
        INTCONbits.INT0IF = 0;

        // Bouton 0
        SERVO_machine(EINT0);
    }

    // Est-ce bien une interruption de INT1?
    if (INTCON3bits.INT1IF) {
        // Abaisse le drapeau:
        INTCON3bits.INT1IF = 0;

        // Bouton 0
        SERVO_machine(EINT1);
    }

    // Est-ce bien une interruption du temporisateur 2?
    if (PIR1bits.TMR2IF) {
        // Abaisse le drapeau du temporisateur 2:
        PIR1bits.TMR2IF = 0;

        // Gère la séquence PWM
        PWM_sequence();

        // Envoie l'horloge à la machine d'états:
        SERVO_machine(TICTAC);
    }
}

/**
 * Point d'entrée.
 */
void main() {
    ANSELA = 0x00;      // Désactive les convertisseurs A/D.
    ANSELB = 0x00;      // Désactive les convertisseurs A/D.
    ANSELC = 0x00;      // Désactive les convertisseurs A/D.

    TRISA = 0x00;       // Tous les bits du port A comme sorties.

    CCPTMRS1bits.C5TSEL = 0;    // CCP5 branché sur tmr2
    T2CONbits.T2CKPS = 1;       // Diviseur de fréq. pour tmr2 sur 32
    T2CONbits.TMR2ON = 1;       // Active le tmr2
    PR2 = 125;                  // Période du tmr2.
    CCP5CONbits.CCP5M = 0xF;    // Active le CCP5.
    TRISAbits.RA4 = 0;          // Active la sortie du CCP5.

    RCONbits.IPEN = 1;          // Active les interruptions.
    INTCONbits.GIEH = 1;        // Active les int. de haute priorité.
    PIE1bits.TMR2IE = 1;        // Active les int. pour temp. 2
    IPR1bits.TMR2IP = 1;        // Int. du temp2 sont de haute prio.

    // Active les résistances de tirage sur INT0 et INT1:
    INTCON2bits.RBPU = 0;
    WPUBbits.WPUB0 = 1;
    WPUBbits.WPUB1 = 1;
    TRISBbits.RB0 = 1;          // Port RB0 en entrée.
    TRISBbits.RB1 = 1;          // Port RB1 en entrée.

    // Active les interruptions externes INT0 et INT1:
    INTCONbits.INT0IE = 1;
    INTCON3bits.INT1IE = 1;
    INTCON3bits.INT1IP = 1;
    INTCON2bits.INTEDG0 = 0;
    INTCON2bits.INTEDG1 = 0;

    // Dodo
    while(1);
}