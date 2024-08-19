import numpy as np
PI4 = 3.141592653589793238462643383*0.25

def yiq_to_rgb(yiq):
    yiq_to_rgb_matrix = np.array([
        [1.0,  0.956,  0.619],
        [1.0, -0.272, -0.647],
        [1.0, -1.106,  1.703]
    ])
    yiq = np.asarray(yiq)
    rgb = np.dot(yiq, yiq_to_rgb_matrix.T)
    rgb = np.clip(rgb, 0, 1)

    return (rgb*255.0).astype(np.uint8)

def rgb_to_yiq(rgb):
    rgb = rgb.astype(np.float64)/255.0
    rgb_to_yiq_matrix = np.array([
        [0.299, 0.587, 0.114],
        [0.5959, -0.2746, -0.3213],
        [0.2115, -0.5227, 0.3112]
    ])
    yiq = np.dot(rgb, rgb_to_yiq_matrix.T)
    yiq = np.clip(yiq, 0, 1)
    return yiq

serieses = [
    np.array([0,0,0,0]),
    np.array([0,0,0,1]),
    np.array([0,0,1,0]),
    np.array([0,0,1,1]),
    np.array([0,1,0,0]),
    np.array([0,1,0,1]),
    np.array([0,1,1,0]),
    np.array([0,1,1,1]),
    np.array([1,0,0,0]),
    np.array([1,0,0,1]),
    np.array([1,0,1,0]),
    np.array([1,0,1,1]),
    np.array([1,1,0,0]),
    np.array([1,1,0,1]),
    np.array([1,1,1,0]),
    np.array([1,1,1,1]),
]

for series in serieses:
    uc = rgb_to_yiq(np.array([255,85,85]))
    uc[1] = uc[1]/0.5957
    uc[2] = uc[2]/0.5226
    
    ul_fft = np.array([0+0j,0+0j,0+0j,0+0j])
    
    ul_fft[0] = uc[0]+0*1j
    ul_fft[1] = (uc[1]-uc[2])*0.5 + (uc[1]+uc[2])*0.5*1j
    
    result = np.fft.fft(ul_fft, norm="backward")

    #print(ul_fft)    
    #print((result.real+1.0)*0.5)
    
    result = (result.real+1.0)*0.5
    
    fft_result = np.fft.fft(series*result) / len(series)
    constant_amplitude = fft_result[0].real

    f = fft_result[1]
    
    yiq = [fft_result[0].real, f.real-f.imag, -f.real-f.imag]
    yiq[1] = yiq[1]*0.5957
    yiq[2] = yiq[2]*0.5226
    
    rgb = yiq_to_rgb(yiq)
    
    html_color = "#{:02X}{:02X}{:02X}".format(rgb[0], rgb[1], rgb[2])

    print(yiq, " -> ", html_color)

#print(f"Y,I,Q = [{constant_amplitude}, {np.sin(first_frequency_phase)}, {-np.cos(first_frequency_phase)}]")

#axes: [I,Q]

#1100 -> [1,0]
#1110 -> [sqrt2, sqrt2]
#0110 -> [0,1]
#0011 -> [-1,0]
#1001 -> [0,-1]
