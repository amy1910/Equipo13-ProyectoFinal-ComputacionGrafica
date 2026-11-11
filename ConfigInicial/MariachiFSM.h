#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

enum class EstadoMusico { Preparado, EjecutandoA, EjecutandoB, Pausa };

inline const char* EstadoMusicoNombre(EstadoMusico estado) {
    switch (estado) {
    case EstadoMusico::Preparado: return "PREPARADO";
    case EstadoMusico::EjecutandoA: return "EJECUTANDO A";
    case EstadoMusico::EjecutandoB: return "EJECUTANDO B";
    case EstadoMusico::Pausa: return "PAUSA";
    default: return "DESCONOCIDO";
    }
}

inline const char* EstadoMusicoNombreCorto(EstadoMusico estado) {
    switch (estado) {
    case EstadoMusico::Preparado: return "IDLE";
    case EstadoMusico::EjecutandoA: return "TOCANDO A";
    case EstadoMusico::EjecutandoB: return "TOCANDO B";
    case EstadoMusico::Pausa: return "PAUSA";
    default: return "N/D";
    }
}

struct MariachiFSM {
    EstadoMusico estado = EstadoMusico::Preparado;
    float tiempo = 0.0f;
    float duracion = 0.35f;
    float fase = 0.0f;
    float anguloBrazo = 0.0f;
    float anguloInstrumento = 0.0f;
    float objetivoBrazo = 0.0f;
    float objetivoInstrumento = 0.0f;

    void Reset() {
        estado = EstadoMusico::Preparado;
        tiempo = 0.0f;
        fase = 0.0f;
        anguloBrazo = 0.0f;
        anguloInstrumento = 0.0f;
        objetivoBrazo = 0.0f;
        objetivoInstrumento = 0.0f;
    }

    const char* EstadoNombre() const {
        return EstadoMusicoNombre(estado);
    }

    const char* EstadoNombreCorto() const {
        return EstadoMusicoNombreCorto(estado);
    }

    void Update(float dt, int tipo) {
        tiempo += dt;
        fase += dt;
        switch (estado) {
        case EstadoMusico::Preparado:
            objetivoBrazo = 0.0f;
            objetivoInstrumento = 0.0f;
            if (tiempo >= 0.25f) { tiempo=0; estado=EstadoMusico::EjecutandoA; }
            break;
        case EstadoMusico::EjecutandoA:
            objetivoBrazo = (tipo == 2 ? -12.0f : -24.0f);
            objetivoInstrumento = (tipo == 2 ? -3.0f : 5.0f);
            if (tiempo >= duracion) { tiempo=0; estado=EstadoMusico::EjecutandoB; }
            break;
        case EstadoMusico::EjecutandoB:
            objetivoBrazo = (tipo == 2 ? 12.0f : 24.0f);
            objetivoInstrumento = (tipo == 2 ? 3.0f : -5.0f);
            if (tiempo >= duracion) { tiempo=0; estado=EstadoMusico::Pausa; }
            break;
        case EstadoMusico::Pausa:
            objetivoBrazo = 0.0f;
            objetivoInstrumento = 0.0f;
            if (tiempo >= 0.12f) { tiempo=0; estado=EstadoMusico::EjecutandoA; }
            break;
        }

        const float clampedDt = std::fmin(std::fmax(dt, 0.0f), 0.05f);
        const float normalized = std::fmin(clampedDt * 10.0f, 1.0f);
        const float smooth = normalized * normalized * (3.0f - 2.0f * normalized);
        anguloBrazo += (objetivoBrazo - anguloBrazo) * smooth;
        anguloInstrumento += (objetivoInstrumento - anguloInstrumento) * smooth;
    }
};

inline glm::mat4 TransformarMariachi(const glm::mat4& base, float angulo,
                                      const glm::vec3& pivote) {
    glm::mat4 m = glm::translate(base, pivote);
    m = glm::rotate(m, glm::radians(angulo), glm::vec3(0,0,1));
    return glm::translate(m, -pivote);
}

inline glm::mat4 TransformarMariachi3D(const glm::mat4& base, float angleZ,
    float angleX, const glm::vec3& pivote) {
    glm::mat4 m = glm::translate(base, pivote);
    m = glm::rotate(m, glm::radians(angleX), glm::vec3(1, 0, 0));
    m = glm::rotate(m, glm::radians(angleZ), glm::vec3(0, 0, 1));
    return glm::translate(m, -pivote);
}
