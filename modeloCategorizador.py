import cv2
from ultralytics import YOLO

# Carga el modelo entrenado
model = YOLO('modelo/modeloYOLO.pt')

# Inicia la captura de video (0 para la cámara web predeterminada)
cam = cv2.VideoCapture(0)

# Verifica si la cámara se ha abierto correctamente
if not cam.isOpened():
    print("No se puede abrir la cámara")
    exit()

# Obtener las dimensiones del video (opcional)
frame_width = int(cam.get(cv2.CAP_PROP_FRAME_WIDTH))
frame_height = int(cam.get(cv2.CAP_PROP_FRAME_HEIGHT))

while True:
    ret, frame = cam.read()
    if not ret:
        print("No se recibió fotograma (fin del video o error)")
        break

    # Realiza la inferencia en el fotograma
    results = model(frame)

    # Obtiene la imagen con las detecciones
    annotated_frame = results[0].plot()

    # Muestra el fotograma con las detecciones
    cv2.imshow('Detecciones', annotated_frame)

    # Salir si se presiona la tecla 'q'
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Libera los recursos
cam.release()
cv2.destroyAllWindows()
