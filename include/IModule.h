// IModule.h — contrato comum de módulos do dispositivo.
//
// Regra de arquitetura (ver PLANO §4/§6):
//   UI -> API (valida autorização/modo/parâmetros) -> dispatcher publica evento
//   -> módulo executa -> evento retorna status.
// Nenhum acesso direto da UI a periféricos; nenhum handler deve bloquear com
// delay(). Use estado + millis() ou tarefas curtas.
#pragma once

namespace mak {

class IModule {
 public:
  virtual ~IModule() = default;

  // Inicializa o módulo. Retorna false em falha (o App decide ERROR_SAFE).
  virtual bool begin() = 0;

  // Passo não bloqueante chamado no loop principal.
  virtual void loop() = 0;

  // Libera recursos / entra em estado seguro.
  virtual void stop() = 0;

  // Nome curto e estável para logs e para o endpoint /api/v1/modules.
  virtual const char* name() const = 0;
};

}  // namespace mak
