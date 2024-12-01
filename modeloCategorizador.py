import cv2
from ultralytics import YOLO
import subprocess

def execute_curl_request(signal):
    # La cadena original con "plastic"
    curl_command = '''curl -X POST http://192.168.4.1/sort \
-H "X-Request-ID: 12345" \
-H "Content-Type: application/json" \
-H "Content-Length: 33" \
-d '{"data": {"grupo": "plastic"}}' '''
    
    # Reemplazar "plastic" por el valor de "signal"
    modified_command = curl_command.replace("plastic", signal)
    
    # Ejecutar el comando modificado usando subprocess
    subprocess.run(modified_command, shell=True, check=True)




# Carga el modelo entrenado
model = YOLO('modelo/modeloYOLO.pt')

# Inicia la captura de video
cam = cv2.VideoCapture(0)

while True:
    ret, frame = cam.read()
    if not ret:
        break

    # Realiza la inferencia
    results = model(frame)

    # Accede al primer resultado (ya que results es una lista)
    result = results[0]

    # Obtiene las cajas detectadas
    boxes = result.boxes

    for box in boxes:
        # Coordenadas de la caja delimitadora
        x1, y1, x2, y2 = box.xyxy[0].tolist()

        # Confianza de la detección
        confidence = box.conf[0].item()

        # Índice de la clase predicha
        class_id = int(box.cls[0].item())

        # Nombre de la clase
        class_name = model.names[class_id]

        # Formatea los datos como cadenas de texto
        detection_info = f"Clase: {class_name}, Confianza: {confidence:.2f}, Coordenadas: ({x1:.0f}, {y1:.0f}, {x2:.0f}, {y2:.0f})"

        # Imprime la información de la detección
        print(f"********* {type(detection_info)} ***************")
        print(detection_info)

        

        # Ejemplo de uso
        #signal_value = "bio"  # Puedes cambiar esto a cualquier valor que desees
        #execute_curl_request(signal_value)

        # Opcional: Dibujar la caja y la etiqueta en el fotograma
        cv2.rectangle(frame, (int(x1), int(y1)), (int(x2), int(y2)), (0, 255, 0), 2)
        cv2.putText(frame, f"{class_name} {confidence:.2f}", (int(x1), int(y1) - 10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.9, (36,255,12), 2)

    # Muestra el fotograma con las detecciones
    cv2.imshow('Detecciones', frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cam.release()
cv2.destroyAllWindows()
