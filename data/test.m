%%
rgb = imread('10084394_0.ppm');
imshow(rgb)
%%
depth1 = imread('10084396_0.pgm');
imshow(depth1)
imagesc(depth1)

realDepth1 = depth1/8;

imagesc(realDepth1)
imshow(realDepth1)
%%
fid = fopen('10084396_0.pgm');

line = fgetl(fid);
assert(strcmp(line,'P5'));

line = fgetl(fid); 
assert(strcmp(line(1),'#'));

line = fgetl(fid);
[width, height] = strtok(line);
width = str2double(width);
height = str2double(height);

line = fgetl(fid);
assert(strcmp(line,'65535'));

depth = fread(fid,Inf,'*uint16');
assert(numel(depth) == width*height);

fclose(fid);

depth = reshape(depth,width,height)';
%%
imshow(depth)
imagesc(depth)

realDepth = depth/8;
imshow(realDepth,[])


