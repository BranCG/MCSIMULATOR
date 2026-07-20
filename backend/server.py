import base64
import os
import wave
import json
import tempfile
from flask import Flask, request, jsonify

app = Flask(__name__)

# Configuración de clientes de Inteligencia Artificial
GEMINI_KEY = os.environ.get("GEMINI_API_KEY", "")
OPENAI_KEY = os.environ.get("OPENAI_API_KEY", "")

# Intentar inicializar Gemini si hay clave disponible
genai_client = None
if GEMINI_KEY:
    try:
        import google.generativeai as genai
        genai.configure(api_key=GEMINI_KEY)
        genai_client = genai
        print("Backend configurado con Google Gemini Flash (Audio NATIVO).")
    except ImportError:
        print("Librería 'google-generativeai' no instalada. Ejecuta: pip install google-generativeai")

# Intentar inicializar OpenAI como alternativa
openai_client = None
if OPENAI_KEY and not genai_client:
    try:
        from openai import OpenAI
        openai_client = OpenAI(api_key=OPENAI_KEY)
        print("Backend configurado con OpenAI Whisper + GPT-4o.")
    except ImportError:
        print("Librería 'openai' no instalada.")


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

        # 2. Crear archivo .wav temporal con encabezados RIFF
        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as temp_wav:
            wav_path = temp_wav.name

        with wave.open(wav_path, 'wb') as wav_file:
            wav_file.setnchannels(channels)
            wav_file.setsampwidth(2) # 16-bit PCM = 2 bytes por muestra
            wav_file.setframerate(sample_rate)
            wav_file.writeframes(raw_pcm_bytes)

        analysis_json = {}

        # ----------------------------------------------------
        # OPCIÓN 1: PROCESAMIENTO NATIVO CON GOOGLE GEMINI FLASH
        # ----------------------------------------------------
        if genai_client:
            print("Procesando audio directamente con Gemini 1.5/2.0 Flash...")
            
            # Subir archivo de audio a la API de Gemini
            audio_file = genai_client.upload_file(path=wav_path)

            prompt = (
                f"Analiza este audio de presentación oral en el contexto de '{context}'. "
                "Transcribe con exactitud lo que dice el usuario y evalúa su desempeño como juez experto en oratoria. "
                "Responde ÚNICAMENTE en formato JSON estricto con la siguiente estructura:\n"
                "{\n"
                '  "transcript": "Texto completo transcrito del audio",\n'
                '  "overall_score": 85.0,\n'
                '  "coherence_score": 90.0,\n'
                '  "filler_words_count": 3,\n'
                '  "nervousness_feedback": "Comentario sobre muletillas, pausas, tono y velocidad de habla.",\n'
                '  "semantic_feedback": "Evaluación del dominio conceptual y claridad argumentativa.",\n'
                '  "recommendations": ["Consejo 1", "Consejo 2", "Consejo 3"]\n'
                "}"
            )

            model = genai_client.GenerativeModel("gemini-1.5-flash")
            response = model.generate_content(
                [audio_file, prompt],
                generation_config={"response_mime_type": "application/json"}
            )

            analysis_json = json.loads(response.text)

            # Limpiar archivo de la nube de Gemini
            try:
                genai_client.delete_file(audio_file.name)
            except Exception:
                pass

        # ----------------------------------------------------
        # OPCIÓN 2: PROCESAMIENTO CON OPENAI WHISPER + GPT-4o
        # ----------------------------------------------------
        elif openai_client:
            print("Procesando audio con OpenAI Whisper + GPT-4o...")
            with open(wav_path, "rb") as audio_file:
                transcription_response = openai_client.audio.transcriptions.create(
                    model="whisper-1",
                    file=audio_file,
                    language="es"
                )
            
            transcript_text = transcription_response.text

            system_prompt = (
                "Eres un juez experto en oratoria. Evalúa la transcripción enviada. "
                "Responde ÚNICAMENTE en formato JSON estricto con: transcript, overall_score, coherence_score, "
                "filler_words_count, nervousness_feedback, semantic_feedback, recommendations (array de 3 strings)."
            )
            user_prompt = f"Contexto: {context}\nTranscripción: \"{transcript_text}\""

            ai_response = openai_client.chat.completions.create(
                model="gpt-4o",
                response_format={"type": "json_object"},
                messages=[
                    {"role": "system", "content": system_prompt},
                    {"role": "user", "content": user_prompt}
                ]
            )
            analysis_json = json.loads(ai_response.choices[0].message.content)

        else:
            # Modo fallback de desarrollo si no hay API keys cargadas aún
            print("AVISO: No se encontró GEMINI_API_KEY u OPENAI_API_KEY. Usando respuesta simulada.")
            analysis_json = {
                "transcript": "Esta es una prueba de voz grabada desde el simulador de realidad virtual en Meta Quest.",
                "overall_score": 88.5,
                "coherence_score": 92.0,
                "filler_words_count": 2,
                "nervousness_feedback": "Excelente ritmo de voz. Se detectaron 2 pequeñas pausas vacilantes al inicio.",
                "semantic_feedback": "Estructura argumentativa muy clara y buena proyección vocal.",
                "recommendations": [
                    "Mantén la mirada fija en los jurados del centro.",
                    "Haz pausas de respiración de 2 segundos entre bloques.",
                    "Proyecta mayor volumen al cerrar tus conclusiones."
                ]
            }

        # Eliminar archivo temporal local
        if os.path.exists(wav_path):
            os.remove(wav_path)

        # Devolver respuesta JSON formateada para Unreal Engine
        return jsonify({
            "transcript": analysis_json.get("transcript", "Transcripción procesada."),
            "overall_score": float(analysis_json.get("overall_score", 85.0)),
            "coherence_score": float(analysis_json.get("coherence_score", 90.0)),
            "filler_words_count": int(analysis_json.get("filler_words_count", 0)),
            "nervousness_feedback": str(analysis_json.get("nervousness_feedback", "Buen ritmo.")),
            "semantic_feedback": str(analysis_json.get("semantic_feedback", "Buena estructura.")),
            "recommendations": list(analysis_json.get("recommendations", ["Practica pausas silenciosas."]))
        }), 200

    except Exception as e:
        print(f"Error procesando audio: {str(e)}")
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    print("Iniciando servidor backend para MC Simulator en http://127.0.0.1:5000...")
    app.run(host='0.0.0.0', port=5000, debug=True)
