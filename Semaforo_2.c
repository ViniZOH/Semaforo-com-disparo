#include <stdio.h>
#include "pico/stdlib.h"

// ---------- Definições de pinos (ajuste conforme seu hardware) ----------
#define LED_VERMELHO   13   // LED VERMELHO no pino GP13
#define LED_AZUL       12   // LED AZUL no pino GP12
#define LED_VERDE      11   // LED VERDE no pino GP11
#define BOTAO   5   // Botão no pino GP5 (usar pull-up)

// ---------- Variáveis globais de controle ----------
static bool sequence_active = false; // Indica se estamos no meio da sequência
static int step = 0;                 // Etapa da sequência (0=parado; 1..3=intermediárias)

/**
 * @brief Função de debounce: retorna true quando detecta
 *        que o botão foi efetivamente "pressionado e solto".
 *
 * Nesta configuração com pull-up:
 *   - Em repouso, BOTAO lê '1'.
 *   - Ao pressionar, passa a '0'.
 *
 * Fluxo de verificação:
 *   1) Se detectamos '0', aguardamos 50 ms para confirmar.
 *   2) Se ainda estiver '0', consideramos "pressionado".
 *   3) Esperamos até voltar a '1' (solto).
 *   4) Retornamos true para sinalizar um clique completo.
 *
 * @return true  se houve clique validado (pressionado + solto)
 * @return false caso contrário
 */
bool botao_pressionado_debounce(void) {
    // Se o botão não está em '0', sai imediatamente (não pressionou)
    if (gpio_get(BOTAO) != 0) {
        return false;
    }

    // Espera 50ms para confirmar se ainda está pressionado
    sleep_ms(50);

    // Checa novamente se continua '0'
    if (gpio_get(BOTAO) == 0) {
        // Agora aguardamos até que o usuário solte o botão
        // (pino volte a '1')
        while (gpio_get(BOTAO) == 0) {
            sleep_ms(10);
        }
        // Ao sair daqui, houve um "pressionar e soltar" confirmado
        return true;
    }

    // Se não continuar '0', foi ruído (rebote)
    return false;
}

/**
 * @brief Callback do alarme one-shot.
 *
 * Chamado em intervalos de 3 segundos, dependendo do 'step'.
 * Cada chamada ajusta os LEDs conforme a etapa e agenda
 * o próximo passo, até finalizar.
 *
 * @param id  ID do alarme (não usado aqui)
 * @param user_data  Ponteiro de dados do usuário (não usado)
 * @return false => "one-shot" (não se repete automaticamente)
 */
int64_t alarm_callback(alarm_id_t id, void *user_data) {
    switch (step) {
        case 1:
            // Depois de 3s do estado "3 LEDs ligados"
            // Deixamos 2 LEDs ligados
            gpio_put(LED_VERMELHO, 1);
            gpio_put(LED_AZUL, 1);
            gpio_put(LED_VERDE, 0);
            printf("Callback: 2 LEDs ligados.\n");

            step = 2;  // Próxima etapa
            add_alarm_in_ms(3000, alarm_callback, NULL, false);
            break;

        case 2:
            // Depois de mais 3s,
            // Deixamos 1 LED ligado
            gpio_put(LED_VERMELHO, 1);
            gpio_put(LED_AZUL, 0);
            gpio_put(LED_VERDE, 0);
            printf("Callback: 1 LED ligado.\n");

            step = 3;
            add_alarm_in_ms(3000, alarm_callback, NULL, false);
            break;

        case 3:
            // Depois de mais 3s,
            // Desligamos todos os LEDs
            gpio_put(LED_VERMELHO, 0);
            gpio_put(LED_AZUL, 0);
            gpio_put(LED_VERDE, 0);
            printf("Callback: TODOS LEDs desligados.\n");

            step = 0;              // Volta ao estado "parado"
            sequence_active = false; // Libera novo disparo via botão
            break;
    }
    // Retorna false pois é um "one-shot"
    return false;
}

int main() {
    // Inicializa a UART padrão (para printf) e o sistema de tempo
    stdio_init_all();

    // ------------------ Configuração de GPIO ------------------
    // LEDs como saída
    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    gpio_init(LED_AZUL);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);

    // Botão como entrada com pull-up interno
    gpio_init(BOTAO);
    gpio_set_dir(BOTAO, GPIO_IN);
    gpio_pull_up(BOTAO); 
    // -> Em repouso: pin = 1
    // -> Pressionando: pin = 0 (GND)

    // Apaga todos os LEDs inicialmente
    gpio_put(LED_VERMELHO, 0);
    gpio_put(LED_AZUL, 0);
    gpio_put(LED_VERDE, 0);

    printf("Sistema iniciado. Aguardando clique no botao (pull-up)...\n");

    // ------------------ Loop Principal ------------------
    while (true) {
        // Se não estamos no meio de uma sequência
        // e detectarmos um clique confirmado no botão:
        if (!sequence_active && botao_pressionado_debounce()) {
            // Inicia a sequência
            sequence_active = true; 
            step = 1; // Primeira etapa (3 LEDs)

            // Liga imediatamente 3 LEDs
            gpio_put(LED_VERMELHO, 1);
            gpio_put(LED_AZUL, 1);
            gpio_put(LED_VERDE, 1);
            printf("Botao pressionado: 3 LEDs ligados (iniciando sequencia).\n");

            // Agenda o próximo estado em 3 segundos (3000 ms)
            add_alarm_in_ms(3000, alarm_callback, NULL, false);
        }

        // Pequeno atraso para não saturar a CPU
        sleep_ms(10);
    }

    return 0; }
