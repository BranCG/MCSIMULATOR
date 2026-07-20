import base64
import os
import wave
import tempfile
from flask import Flask, request, jsonify
from openai import OpenAI

app = Flask(__name__)

# Instanciar cliente de OpenAI (asegúrate de tener la variable de entorno OPENAI_API_KEY)
client = OpenAI(api_key=os.environ.get("OPENAI_API_KEY", "TU_OPENAI_API_KEY_AQUI"))

@app.route('/api/analyze', methods=['POST'])
def analyze_speech():
    try:
        data = request.get_json()
        if not data or 'audio_base64' not in data:
            return jsonify({"error": "No audio payload received"}), 400

        audio_b64 = data['audio_base64']
        context = data.get('context', 'General')
        sample_rate = int(data.get('sample_rate', 16000))
        channels = int(data.get('channels', 1))

        # 1. Decodificar Base64 a bytes crudos PCM 16-bit
        raw_pcm_bytes = base64.b64decode(audio_b64)

        if len(raw_pcm_bytes) == 0:
            return jsonify({"error": "Audio buffer is empty"}), 400

        # 2. Crear archivo .wav temporal con encabezados RIFF legítimos
        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as temp_wav:
            wav_path = temp_wav.name

        with wave.open(wav_path, 'wb') as wav_file:
            wav_file.setnchannels(channels)
            wav_file.setsampwidth(2) # 16-bit PCM = 2 bytes por muestra
            wav_file.setframerate(sample_rate)
            wav_file.writeframes(raw_pcm_bytes)

        # 3. Transcribir audio usando OpenAI Whisper API
        with open(wav_path, "rb") as audio_file:
            transcription_response = client.audio.transcriptions.create(
                model="whisper-1",
                file=audio_file,
                language="es" # Idioma español por defecto
            )
        
        transcript_text = transcription_response.text

        # Eliminar archivo temporal
        if os.path.exists(wav_path):
            os.remove(wav_path)

        # 4. Evaluación semántica y de nerviosismo usando GPT-4o
        system_prompt = (
            "Eres un juez experto en oratoria, lenguaje corporal y evaluación de presentaciones académicas y ejecutivas. "
            "Debes analizar la transcripción enviada por un usuario en un simulador de Realidad Virtual y evaluar su desempeño. "
            "Responde ÚNICAMENTE en formato JSON estricto con las siguientes llaves:\n"
            "- transcript: (String) La transcripción completa recibida.\n"
            "- overall_score: (Float de 0.0 a 100.0) Calificación general.\n"
            "- coherence_score: (Float de 0.0 a 100.0) Nivel de cohesión lógica y terminología.\n"
            "- filler_words_count: (Int) Conteo aproximado de muletillas (ej: 'este', 'eh', 'bueno', 'o sea').\n"
            "- nervousness_feedback: (String) Crítica constructiva sobre ritmo, dudas o vacilaciones.\n"
            "- semantic_feedback: (String) Evaluación de la claridad argumentativa y dominio técnico.\n"
            "- recommendations: (Array de Strings) 3 consejos breves y accionables para mejorar."
        )

        user_prompt = f"Contexto de la presentación: {context}\nTranscripción del usuario: \"{transcript_text}\""

        ai_response = client.chat.completions.create(
            model="gpt-4o",
            response_format={"type": "json_object"},
            messages=[
                {"role": "system", "content": system_prompt},
                {"role": "user", "content": user_prompt}
            ],
            temperature=0.3
        )

        import json
        analysis_json = json.loads(ai_response.choices[0].message.content)

        # Devolver el formato exacto que espera Unreal Engine SpeechAnalyzer.cpp
        return jsonify({
            "transcript": analysis_json.get("transcript", transcript_text),
            "overall_score": float(analysis_json.get("overall_score", 80.0)),
            "coherence_score": float(analysis_json.get("coherence_score", 85.0)),
            "filler_words_count": int(analysis_json.get("filler_words_count", 0)),
            "nervousness_feedback": str(analysis_json.get("nervousness_feedback", "Buen ritmo general.")),
            "semantic_feedback": str(analysis_json.get("semantic_feedback", "Contenido estructurado correctamente.")),
            "recommendations": list(analysis_json.get("recommendations", ["Practica pausas silenciosas."]))
        }), 200

    except Exception as e:
        print(f"Error procesando audio: {str(e)}")
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    print("Iniciando servidor de Inteligencia Artificial para MC Simulator en http://127.0.0.1:5000...")
    app.run(host='0.0.0.0', port=5000, debug=True)
