from PIL import Image, ImageTk
import tkinter as tk
import numpy as np
PI4 = 3.141592653589793238462643383*0.25

    
cga_colors = [
    np.array([0, 0, 0]),        # Black
    np.array([90, 56, 255]),      # Blue
    np.array([30, 144, 0]),      # Green
    np.array([0, 137, 91]),    # Cyan
    np.array([179, 38, 89]),      # Red
    np.array([144, 31, 215]),    # Magenta
    np.array([86, 119, 0]),     # Brown
    np.array([183, 187, 182]),  # Light Gray
    np.array([72, 68, 72]),     # Dark Gray
    np.array([160, 126, 255]),    # Light Blue
    np.array([100, 214, 32]),    # Light Green
    np.array([65, 207, 162]),   # Light Cyan
    np.array([249, 108, 159]),    # Light Red
    np.array([214, 101, 255]),   # Light Magenta
    np.array([159, 189, 0]),   # Yellow
    np.array([255, 255, 255])   # White
]

def getthing(x,y):
    rgb_to_yiq_matrix = np.array([
        [0.299, 0.587, 0.114],
        [0.5959/0.5959, -0.2746/0.5959, -0.3213/0.5959],
        [0.2115/0.5226, -0.5227/0.5226, 0.3112/0.5226]
    ])
    """last_matrix = np.array([
        [0.25, 0.25, 0.25, 0.25],
        [0.5, 0, -0.5, 0],
        [0, 0.5, 0, -0.5]
    ])
    yiq_to_rgb_matrix = np.array([
        [1.0,  0.956*0.5959,  0.619*0.5226],
        [1.0, -0.272*0.5959, -0.647*0.5226],
        [1.0, -1.106*0.5959,  1.703*0.5226]
    ])
    totalmatrix = np.dot(yiq_to_rgb_matrix, last_matrix)"""
    
    totalmatrix = np.array([
        [ 0.5348402,  0.4117447, -0.0348402,  0.0882553],
        [ 0.1689576,  0.0809389,  0.3310424,  0.4190611],
        [-0.0795327,  0.6949939,  0.5795327, -0.1949939]]
    )
    
    print("totalmatrix")
    print(totalmatrix)

    series = np.array([1,1,0,0])
    #series = np.array([0,1,1,0])
    #series = np.array([0,1,0,1])
    
    #for col in cga_colors:
    #    print(np.dot(col.astype(np.float64)/255.0, rgb_to_yiq_matrix.T))
    #exit(0)

    rgbA = cga_colors[x].astype(np.float64)/255.0
    ucA = np.dot(rgbA, rgb_to_yiq_matrix.T)
    resultA = np.array([ucA[1],ucA[2],-ucA[1],-ucA[2]])+ucA[0]
    
    rgbB = cga_colors[y].astype(np.float64)/255.0
    ucB = np.dot(rgbB, rgb_to_yiq_matrix.T)
    resultB = np.array([ucB[1],ucB[2],-ucB[1],-ucB[2]])+ucB[0]

    combined = series*resultA + (1.0-series)*resultB
    print("---------------")
    print(combined)
    print(totalmatrix.T)
    print("vvvvvvvvvvvvvvvvvvvv")
       
    rgb = np.dot(combined, totalmatrix.T)
    rgb = (np.clip(rgb, 0, 1)*255.0).astype(np.uint8)
    
    return (rgb[0], rgb[1], rgb[2])

MULT = 32

width, height = 16*MULT, 16*MULT
image = Image.new('RGB', (width, height), 'black')

# Access the pixel data
pixels = image.load()

# Modify some pixels
for x in range(0,width,MULT):
    for y in range(0,height,MULT):
        pix = getthing(x//MULT, y//MULT)
        for x2 in range(0,MULT):
            for y2 in range(0,MULT):
                pixels[x+x2, y+y2] = pix
        

# Create a Tkinter window
root = tk.Tk()
root.title("Image Display")

# Convert the Image object into a Tkinter PhotoImage object
photo = ImageTk.PhotoImage(image)

# Create a Label widget to display the image
label = tk.Label(root, image=photo)
label.pack()

# Run the Tkinter event loop
root.mainloop()