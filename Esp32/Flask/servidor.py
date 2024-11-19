from flask import Flask, request, jsonify
from datetime import datetime
import os

app = Flask(__name__)

# Cria o diretório para salvar as fotos, se não existir
if not os.path.exists("uploads"):
    os.makedirs("uploads")

# Variáveis globais para armazenar os dados
soil_moisture_data = []
dht_data = []
water_level_data = []  # Nova variável para o nível de água

# Endpoint para receber dados de umidade do solo
@app.route('/data', methods=['POST'])
def receive_soil_moisture():
    data = request.form.get("message")
    if data:
        soil_moisture_data.append(data)
        print(f"Umidade do solo recebida: {data}")
        return jsonify({"status": "success", "data": data}), 200
    else:
        return jsonify({"status": "failed", "reason": "No data provided"}), 400

# Endpoint para receber dados de temperatura e umidade (DHT11)
@app.route('/endpoint', methods=['POST'])
def receive_dht_data():
    temp = request.form.get("temperature")
    humidity = request.form.get("humidity")
    if temp and humidity:
        dht_data.append({"temperature": temp, "humidity": humidity})
        print(f"Dados de temperatura e umidade recebidos: {temp}°C, {humidity}%")
        return jsonify({"status": "success", "temperature": temp, "humidity": humidity}), 200
    else:
        return jsonify({"status": "failed", "reason": "Incomplete data"}), 400

# Endpoint para receber e salvar fotos
@app.route('/upload', methods=['POST'])
def upload_image():
    if 'file' not in request.files:
        return jsonify({"status": "failed", "reason": "No file part"}), 400

    file = request.files['file']
    if file.filename == '':
        return jsonify({"status": "failed", "reason": "No selected file"}), 400

    # Salva a imagem com um nome baseado na data e hora
    filename = datetime.now().strftime("%Y-%m-%d_%H-%M-%S") + ".jpg"
    file.save(os.path.join("uploads", filename))
    print(f"Imagem recebida e salva como {filename}")
    
    return jsonify({"status": "success", "filename": filename}), 200

# Novo endpoint para receber dados do nível de água
@app.route('/update', methods=['POST'])
def receive_water_level():
    water_level = request.form.get("waterLevel")
    if water_level:
        water_level_data.append(water_level)
        print(f"Nível de água recebido: {water_level}")
        return jsonify({"status": "success", "waterLevel": water_level}), 200
    else:
        return jsonify({"status": "failed", "reason": "No data provided"}), 400

# Página inicial para exibir dados recebidos
@app.route('/')
def home():
    return jsonify({
        "soil_moisture_data": soil_moisture_data,
        "dht_data": dht_data,
        "water_level_data": water_level_data  # Exibe também os dados do nível de água
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
