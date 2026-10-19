#include "CrashDiagnostics.h"
#include <esp_core_dump.h>

namespace 
{
void adicionarEndereco(String& texto, uint32_t valor)
{
    char endereco[12];
    snprintf(endereco, sizeof(endereco), "0x%08lx", (unsigned long)valor);
    texto += endereco;
}
}

String CrashDiagnostics::lerEApagar(const String& motivoReset)
{
    String resultado = "Ultimo reset: " + motivoReset + "\n";
#if CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH && CONFIG_ESP_COREDUMP_DATA_FORMAT_ELF
    esp_err_t erro = esp_core_dump_image_check();
    if (erro != ESP_OK)
        return resultado + "Sem core dump valido: " + esp_err_to_name(erro) + "\n";

    char motivo[200] = {};
    if (esp_core_dump_get_panic_reason(motivo, sizeof(motivo)) == ESP_OK)
        resultado += "Panic salvo: " + String(motivo) + "\n";

    esp_core_dump_summary_t resumo = {};
    erro = esp_core_dump_get_summary(&resumo);
    if (erro != ESP_OK)
        return resultado + "Falha ao ler resumo: " + esp_err_to_name(erro) +
               "\nRegistro interno preservado.\n";

    resultado += "Tarefa: ";
    for (size_t i = 0; i < sizeof(resumo.exc_task) && resumo.exc_task[i]; ++i)
        resultado += resumo.exc_task[i];
    resultado += "\nPC: ";
    adicionarEndereco(resultado, resumo.exc_pc);
#if CONFIG_IDF_TARGET_ARCH_RISCV
    resultado += "\nRA: ";
    adicionarEndereco(resultado, resumo.ex_info.ra);
    resultado += "\nSP: ";
    adicionarEndereco(resultado, resumo.ex_info.sp);
    resultado += "\nMCAUSE: ";
    adicionarEndereco(resultado, resumo.ex_info.mcause);
    resultado += "\nMTVAL: ";
    adicionarEndereco(resultado, resumo.ex_info.mtval);
    resultado += "\nBacktrace: requer analise da pilha com o ELF deste firmware (RISC-V).\nPilha:";
    const size_t tamanho = resumo.exc_bt_info.dump_size < sizeof(resumo.exc_bt_info.stackdump)
        ? resumo.exc_bt_info.dump_size : sizeof(resumo.exc_bt_info.stackdump);
    for (size_t i = 0; i < tamanho; ++i) {
        if (i % 16 == 0) {
            resultado += "\n";
            adicionarEndereco(resultado, resumo.ex_info.sp + i);
            resultado += ":";
        }
        char byteHex[4];
        snprintf(byteHex, sizeof(byteHex), " %02x", resumo.exc_bt_info.stackdump[i]);
        resultado += byteHex;
    }
#else
    resultado += "\nBacktrace:";
    for (uint32_t i = 0; i < resumo.exc_bt_info.depth &&
         i < sizeof(resumo.exc_bt_info.bt) / sizeof(resumo.exc_bt_info.bt[0]); ++i) {
        resultado += " ";
        adicionarEndereco(resultado, resumo.exc_bt_info.bt[i]);
    }
    resultado += "\nBacktrace corrompido: " + String(resumo.exc_bt_info.corrupted ? "SIM" : "NAO");
#endif
    resultado += "\nELF SHA256: ";
    for (size_t i = 0; i < sizeof(resumo.app_elf_sha256) && resumo.app_elf_sha256[i]; ++i)
        resultado += static_cast<char>(resumo.app_elf_sha256[i]);
    resultado += "\n";

    // Mesmo ciclo do central, sem arquivamento em SD: apaga somente apos ler o resumo.
    erro = esp_core_dump_image_erase();
    resultado += erro == ESP_OK ? "Registro de crash apagado apos leitura.\n"
        : "Falha ao apagar registro de crash: " + String(esp_err_to_name(erro)) + "\n";
#else
    resultado += "Core dump nao habilitado neste firmware.\n";
#endif
    return resultado;
}
