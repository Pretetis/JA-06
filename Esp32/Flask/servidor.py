from flask import Flask, request, jsonify, send_file
from datetime import datetime
from pymongo import MongoClient
from gridfs import GridFS
from bson import ObjectId
import os

app = Flask(__name__)

try:
    client = MongoClient('mongodb+srv://tiagong2005:tiagong2005@ja-cluster.nufoa.mongodb.net/?retryWrites=true&w=majority&appName=ja-cluster')  # Substitua pelo URI do seu banco, se necessário
    client.admin.command('ping')
    print("Conexão com o MongoDB estabelecida com sucesso.")
    
except:
    print('Não foi possível se conectar com o banco de dados!')
    exit()
    
if not os.path.exists("uploads"):
    os.makedirs("uploads")
    
db = client['ja-cluster'] 
soil_humidity_collection = db['c_soil_humidity']
temperatureHumidity = db['c_temperature_and_air-humidity']
water_level_collection = db['c_water_level']
camera_images = db['c_images']
fs = GridFS(db)

# humidade do solo
@app.route('/data', methods=['POST'])
def SoilMisture():
    data = request.form.get("humidity")
    if data:
        soil_humidity_collection.insert_one({
            "humidity": data,
            "timestamp": datetime.now().strftime("%d %m %Y - %H %M")
        })
        print(f"Umidade do solo recebida: {data}")
        return jsonify({"status": "success", "data": data}), 200
    else:
        return jsonify({"status": "failed", "reason": "No data provided"}), 400

# temperatura e humidade do ar
@app.route('/endpoint', methods=['POST'])
def Temperature():
    soil_temperature = request.form.get("temperature")
    air_humidity = request.form.get("air-humidity")
    if soil_temperature and air_humidity:
        temperatureHumidity.insert_one({
            "temperature": soil_temperature,
            "air-humidity": air_humidity,
            "timestamp": datetime.now().strftime("%d %m %Y - %H %M")
        })
        print(f"Dados de temperatura e umidade recebidos: {soil_temperature}% {air_humidity}%")
        temperatureHumidityList.append({"temperature": soil_temperature, "air-humidity": air_humidity})
        return jsonify({"status": "success", "air-humidity": soil_temperature, "temperature": air_humidity}), 200
    else:
        return jsonify({"status": "failed", "reason": "Incomplete data"}), 400

# nível da água
@app.route('/update', methods=['POST'])
def WaterLevel():
    water_level = request.form.get("waterLevel")
    if water_level:
        water_level_collection.insert_one({
            "water_level": water_level,
            "timestamp": datetime.now().strftime("%d %m %Y - %H %M")
        })
        print(f"Nível de água recebido: {water_level}")
        return jsonify({"status": "success", "waterLevel": water_level}), 200
    else:
        return jsonify({"status": "failed", "reason": "No data provided"}), 400

@app.route('/upload', methods=['POST'])
def upload_image():
    if 'file' not in request.files:
        return jsonify({"status": "failed", "reason": "No file part"}), 400

    file = request.files['file']
    if file.filename == '':
        return jsonify({"status": "failed", "reason": "No selected file"}), 400

    # Salvar a imagem no sistema de arquivos ou em um serviço de armazenamento
    filename = datetime.now().strftime("%d %m %Y - %H %M") + ".jpg" 
    filepath = os.path.join("uploads", filename)
    file.save(filepath)

    # Salvar o caminho da imagem no MongoDB
    camera_images.insert_one({
        "filename": filename,
        "filepath": filepath,
        "timestamp": datetime.now()
    })

    print(f"Imagem salva no servidor e no MongoDB: {filename}")
    return jsonify({"status": "success", "filename": filename}), 200


# exibir dados recebidos
@app.route('/')
def MainHome():
    soil_moisture = list(soil_humidity_collection.find({}, {"_id": 0}))
    dht = list(temperatureHumidity.find({}, {"_id": 0}))
    water_level = list(water_level_collection.find({}, {"_id": 0}))
    images = list(camera_images.find({}, {"_id": 0}))

    return jsonify({
        "soil_moisture_data": soil_moisture,
        "dht_data": dht,
        "water_level_data": water_level,
        "images-path": images
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
