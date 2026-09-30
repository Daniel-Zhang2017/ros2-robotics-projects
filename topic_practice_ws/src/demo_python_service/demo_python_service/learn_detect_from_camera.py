import face_recognition
import cv2
from ament_index_python.packages import get_package_share_directory

def main():
    # Get resource path (keep for reference, not used for camera)
    default_image_path = get_package_share_directory(
        'demo_python_service') + '/resource/default.jpg'

    # Open webcam, 0 = default camera
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Cannot open camera")
        return

    while True:
        # Read one frame from camera
        ret, frame = cap.read()
        if not ret:
            print("Can't receive frame (stream end?). Exiting ...")
            break

        # face_recognition requires RGB format, OpenCV reads BGR
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        # Detect face locations
        face_locations = face_recognition.face_locations(
            rgb_frame, number_of_times_to_upsample=1, model='hog')

        # Draw bounding boxes on original BGR frame
        for top, right, bottom, left in face_locations:
            cv2.rectangle(frame, (left, top), (right, bottom), (255, 0, 0), 4)

        cv2.imshow('Face Detection', frame)

        # Press q to quit
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Release resources
    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
