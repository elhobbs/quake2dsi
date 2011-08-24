#ifndef SND_7_H_
#define SND_7_H_

void S_Init7 (unsigned int, unsigned int);
void S_AmbientOff7 (void);
void S_AmbientOn7 (void);
void S_Shutdown7 (void);
void S_TouchSound7 (char *sample);
void S_ClearBuffer7 (void);
void S_StaticSound7 (void *sfx, float *origin, float vol, float attenuation);
void S_StartSound7 (int entnum, int entchannel, void *sfx, float *origin, float fvol,  float attenuation);
void S_StopSound7 (int entnum, int entchannel);
void *S_PrecacheSound7 (char *sample);
void S_ClearPrecache7 (void);
void S_Update7 (float *origin, float *v_forward, float *v_right, float *v_up);
void S_StopAllSounds7 (bool clear);
void S_BeginPrecaching7 (void);
void S_EndPrecaching7 (void);
void S_ExtraUpdate7 (void);
void S_LocalSound7 (char *s);
#endif /*SND_7_H_*/
