#include <framebot/framebot.h>

static CURL *curl = NULL;
static CURL *down_curl = NULL;

/* start curl in framebot_init */
void network_init(){
    curl_global_init(CURL_GLOBAL_DEFAULT);
}


MemStore * mem_store() {
    MemStore *mem = (MemStore *)calloc(1, sizeof(MemStore));

    mem->content = (char *)calloc(1, 1);
    mem->size = 0;

    return mem;
}

void mem_store_free(MemStore * memStore) {
    if (memStore->content)
        free(memStore->content);
    
    free(memStore);
}

/* mem_write_callback is response of request Telegram */
size_t mem_write_callback(void *data, size_t size, size_t nmemb, void *userp) {
    size_t finalsz = size * nmemb;
    MemStore *json = (MemStore *)userp;

    json->content = (char *)realloc(json->content, json->size + finalsz + 1);

    if (json->content) {
        strcpy(json->content, (char *)data);
        json->size += finalsz;
        json->content[json->size] = '\0';

        return finalsz;
    }

    return 0;
}

/* send data to telegram */
MemStore * call_method(const char *token, const char *method){
    CURLcode res;
    size_t url_size = API_URL_LEN + strlen( token ) + strlen( method ) + 2;
    char * url = ( char * ) calloc(1,  url_size );

    strcpy( url, API_URL );
    strcat( url, token );
    strcat( url, "/" );
    strcat( url, method );

    url[url_size - 1] = '\0';

    MemStore *buff = mem_store();

    if( !curl ){
        curl = curl_easy_init();
    }
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void *)buff );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, mem_write_callback );
    curl_easy_setopt( curl, CURLOPT_URL, url );

    free( url );

    res = curl_easy_perform( curl );

    if( res == CURLE_OK ) {
        return buff;
    }

    return NULL;
}

int call_method_download(const char * token, char * namefile, File *ofile){
    FILE * binary;
    CURLcode res;
    size_t url_size;
    char *url, *path;

    if(!ofile)
        return 0;

    url_size = API_URL_FILE_LEN + strlen(token) + strlen(ofile->file_path) + 2;
    url = (char *)calloc(1, url_size);

    strcpy(url, API_URL_FILE);
    strcat(url, token);
    strcat(url, "/");
    strcat(url, ofile->file_path);

    url[url_size - 1] = '\0';

    if(!down_curl)
         down_curl = curl_easy_init();

    curl_easy_setopt(down_curl, CURLOPT_URL, url);
    curl_easy_setopt(down_curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(down_curl, CURLOPT_SSL_VERIFYPEER, 0L);

    free(url);

    if(namefile){
        binary = fopen(namefile, "wb");
    }
    else{
        char name[10];
        static int id = 0;
        sprintf(name, "file%d", id++);

        binary = fopen(name, "wb");
    }

    if(binary){
        curl_easy_setopt(down_curl, CURLOPT_WRITEDATA, (void *)binary);
        curl_easy_setopt(down_curl, CURLOPT_WRITEFUNCTION, NULL);
    }
    else{
        return 0;
    }

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(down_curl);
    fclose(binary);

    if (res == CURLE_OK)
        return 1;

    return 0;
}

MemStore * call_method_upload(const char * token, IFile ifile){
    char method[25];
    CURL * curl;
    CURLcode res;
    curl_mime * form = NULL;
    curl_mimepart * field = NULL;
    
    curl = curl_easy_init();
    if(curl) {
        /* Create the form */
        form = curl_mime_init(curl);

        switch(ifile.type){
            case SENDPHOTO:
                strcpy(method, "sendPhoto");

                /* Unique identifier for the target */
                if(ifile.photo.chat_id != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "chat_id");
                    curl_mime_data(field, ifile.photo.chat_id, CURL_ZERO_TERMINATED);
                }

                /* upload file */
                if(ifile.photo.filename != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "photo");
                    if(api_tg_exist(ifile.photo.filename))
                        curl_mime_filedata(field, ifile.photo.filename);
                    else
                        curl_mime_data(field, ifile.photo.filename, CURL_ZERO_TERMINATED);

                }

                /* Photo caption */
                if(ifile.photo.caption != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "caption");
                    curl_mime_data(field, ifile.photo.caption, CURL_ZERO_TERMINATED);
                }

                /* Sends the message silently */
                if(ifile.photo.disable_notification != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "disable_notification");
                    curl_mime_data(field, ifile.photo.disable_notification, CURL_ZERO_TERMINATED);
                }

                /* If the message is a reply, ID of the original message */
                if(ifile.photo.reply_to_message_id != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "reply_to_message_id");
                    curl_mime_data(field, ifile.photo.reply_to_message_id, CURL_ZERO_TERMINATED);
                }

                if(ifile.photo.reply_markup != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "reply_to_message_id");
                    curl_mime_data(field, ifile.photo.reply_markup, CURL_ZERO_TERMINATED);
                }
            break;
            case SENDAUDIO:
                strcpy(method, "sendAudio");

                /* Fill in the file upload field */
                if(ifile.audio.chat_id != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "chat_id");
                    curl_mime_data(field, ifile.audio.chat_id, CURL_ZERO_TERMINATED);
                }

                /* Fill in the filename field */
                if(ifile.audio.filename != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "audio");
                    if(api_tg_exist(ifile.audio.filename))
                        curl_mime_filedata(field, ifile.audio.filename);
                    else
                        curl_mime_data(field, ifile.audio.filename, CURL_ZERO_TERMINATED);
                }

                /* Audio caption */
                if(ifile.audio.caption != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "caption");
                    curl_mime_data(field, ifile.audio.caption, CURL_ZERO_TERMINATED);
                }

                /* Duration of the audio in seconds */
                if(ifile.audio.duration != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "duration");
                    curl_mime_data(field, ifile.audio.duration, CURL_ZERO_TERMINATED);
                }

                /* Performer */
                if(ifile.audio.performer != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "performer");
                    curl_mime_data(field, ifile.audio.performer, CURL_ZERO_TERMINATED);
                }

                /* Track name */
                if(ifile.audio.title != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "title");
                    curl_mime_data(field, ifile.audio.title, CURL_ZERO_TERMINATED);
                }

                /* Sends the message silently */
                if(ifile.audio.disable_notification != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "disable_notification");
                    curl_mime_data(field, ifile.audio.disable_notification, CURL_ZERO_TERMINATED);
                }

                /* If the message is a reply */
                if(ifile.audio.reply_to_message_id != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "reply_to_message_id");
                    curl_mime_data(field, ifile.audio.reply_to_message_id, CURL_ZERO_TERMINATED);
                }

                if(ifile.audio.reply_markup != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "reply_to_message_id");
                    curl_mime_data(field, ifile.audio.reply_markup, CURL_ZERO_TERMINATED);
                }
            break;
            case SENDDOCUMENT:
                strcpy(method, "sendDocument");

                /* Unique identifier for the target */
                if(ifile.document.chat_id != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "chat_id");
                    curl_mime_data(field, ifile.document.chat_id, CURL_ZERO_TERMINATED);
                }

                /* upload file */
                if(ifile.document.filename != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "document");
                    if(api_tg_exist(ifile.document.filename))
                        curl_mime_filedata(field, ifile.document.filename);
                    else
                        curl_mime_data(field, ifile.document.filename, CURL_ZERO_TERMINATED);
                }

                /* Document caption */
                if(ifile.document.caption != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "caption");
                    curl_mime_data(field, ifile.document.caption, CURL_ZERO_TERMINATED);
                }

                /* Sends the message silently */
                if(ifile.document.disable_notification != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "disable_notification");
                    curl_mime_data(field, ifile.document.disable_notification, CURL_ZERO_TERMINATED);
                }

                /* If the message is a reply, ID of the original message */
                if(ifile.document.reply_to_message_id != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "reply_to_message_id");
                    curl_mime_data(field, ifile.document.reply_to_message_id, CURL_ZERO_TERMINATED);
                }

                if(ifile.document.reply_markup != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "reply_to_message_id");
                    curl_mime_data(field, ifile.document.reply_markup, CURL_ZERO_TERMINATED);
                }
            break;
            case SENDVIDEO:
                strcpy(method, "sendVideo");

                /* Unique identifier for the target */
                if(ifile.video.chat_id != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "chat_id");
                    curl_mime_data(field, ifile.video.chat_id, CURL_ZERO_TERMINATED);
                }

                /* upload file */
                if(ifile.video.filename != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "video");
                    if(api_tg_exist(ifile.video.filename))
                        curl_mime_filedata(field, ifile.video.filename);
                    else
                        curl_mime_data(field, ifile.video.filename, CURL_ZERO_TERMINATED);
                }

                /* Document caption */
                if(ifile.video.duration != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "duration");
                    curl_mime_data(field, ifile.video.duration, CURL_ZERO_TERMINATED);
                }

                /* Document caption */
                if(ifile.video.width != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "width");
                    curl_mime_data(field, ifile.video.width, CURL_ZERO_TERMINATED);
                }

                /* Document caption */
                if(ifile.video.height != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "height");
                    curl_mime_data(field, ifile.video.height, CURL_ZERO_TERMINATED);
                }

                /* Document caption */
                if(ifile.video.caption != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "caption");
                    curl_mime_data(field, ifile.video.caption, CURL_ZERO_TERMINATED);
                }

                /* Document parse_mode */
                if(ifile.video.parse_mode != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "parse_mode");
                    curl_mime_data(field, ifile.video.parse_mode, CURL_ZERO_TERMINATED);
                }

                /* Document supports_streaming */
                if(ifile.video.supports_streaming != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "supports_streaming");
                    curl_mime_data(field, ifile.video.supports_streaming, CURL_ZERO_TERMINATED);
                }

                /* Sends the message silently */
                if(ifile.video.disable_notification != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "disable_notification");
                    curl_mime_data(field, ifile.video.disable_notification, CURL_ZERO_TERMINATED);
                }

                /* If the message is a reply, ID of the original message */
                if(ifile.video.reply_to_message_id != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "reply_to_message_id");
                    curl_mime_data(field, ifile.video.reply_to_message_id, CURL_ZERO_TERMINATED);
                }

                if(ifile.video.reply_markup != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "reply_to_message_id");
                    curl_mime_data(field, ifile.video.reply_markup, CURL_ZERO_TERMINATED);
                }
            break;
            case SENDVOICE:
                strcpy(method, "sendVoice");

                /* Fill in the file upload field */
                if(ifile.voice.chat_id != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "chat_id");
                    curl_mime_data(field, ifile.voice.chat_id, CURL_ZERO_TERMINATED);
                }

                /* Fill in the filename field */
                if(ifile.voice.filename != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "voice");
                    if(api_tg_exist(ifile.voice.filename))
                        curl_mime_filedata(field, ifile.voice.filename);
                    else
                        curl_mime_data(field, ifile.voice.filename, CURL_ZERO_TERMINATED);
                }

                /* voice caption */
                if(ifile.voice.caption != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "caption");
                    curl_mime_data(field, ifile.voice.caption, CURL_ZERO_TERMINATED);
                }

                /* Duration of the voice in seconds */
                if(ifile.voice.duration != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "duration");
                    curl_mime_data(field, ifile.voice.duration, CURL_ZERO_TERMINATED);
                }

                /* Sends the message silently */
                if(ifile.voice.disable_notification != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "disable_notification");
                    curl_mime_data(field, ifile.voice.disable_notification, CURL_ZERO_TERMINATED);
                }

                /* If the message is a reply */
                if(ifile.voice.reply_to_message_id != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "reply_to_message_id");
                    curl_mime_data(field, ifile.voice.reply_to_message_id, CURL_ZERO_TERMINATED);
                }

                if(ifile.voice.reply_markup != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "reply_to_message_id");
                    curl_mime_data(field, ifile.voice.reply_markup, CURL_ZERO_TERMINATED);
                }
            break;
            case SENDVIDEONOTE:
                strcpy(method, "sendVideoNote");

                /* Fill in the file upload field */
                if(ifile.videonote.chat_id != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "chat_id");
                    curl_mime_data(field, ifile.videonote.chat_id, CURL_ZERO_TERMINATED);
                }

                /* Fill in the filename field */
                if(ifile.videonote.filename != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "video_note");
                    if(api_tg_exist(ifile.videonote.filename))
                        curl_mime_filedata(field, ifile.videonote.filename);
                    else
                        curl_mime_data(field, ifile.videonote.filename, CURL_ZERO_TERMINATED);
                }

                /* Duration of the voice in seconds */
                if(ifile.videonote.duration != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "duration");
                    curl_mime_data(field, ifile.videonote.duration, CURL_ZERO_TERMINATED);
                }

                /* Duration of the voice in seconds */
                if(ifile.videonote.length != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "duration");
                    curl_mime_data(field, ifile.videonote.length, CURL_ZERO_TERMINATED);
                }

                /* Sends the message silently */
                if(ifile.videonote.disable_notification != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "disable_notification");
                    curl_mime_data(field, ifile.videonote.disable_notification, CURL_ZERO_TERMINATED);
                }

                /* If the message is a reply */
                if(ifile.videonote.reply_to_message_id != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "reply_to_message_id");
                    curl_mime_data(field, ifile.videonote.reply_to_message_id, CURL_ZERO_TERMINATED);
                }

                if(ifile.videonote.reply_markup != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "reply_to_message_id");
                    curl_mime_data(field, ifile.videonote.reply_markup, CURL_ZERO_TERMINATED);
                }
            break;
            case SETCHATPHOTO:
                strcpy(method, "setChatPhoto");

                /* Fill in the file upload field */
                if(ifile.chatphoto.chat_id != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "chat_id");
                    curl_mime_data(field, ifile.chatphoto.chat_id, CURL_ZERO_TERMINATED);
                }

                /* Fill in the filename field */
                if(ifile.videonote.filename != NULL){
                    field = curl_mime_addpart(form);
                    curl_mime_name(field, "video_note");
                    if(api_tg_exist(ifile.chatphoto.filename))
                        curl_mime_filedata(field, ifile.chatphoto.filename);
                    else
                        curl_mime_data(field, ifile.chatphoto.filename, CURL_ZERO_TERMINATED);
                }
            break;

        }

        MemStore * buff = NULL;;
        size_t url_size;
        char * url = NULL;

        buff = mem_store();
        url_size = API_URL_LEN + strlen(token) + strlen(method) + 2;
        url = calloc(1, url_size);

        strcpy(url, API_URL);
        strcat(url, token);
        strcat(url, "/");
        strcat(url, method);

        url[url_size - 1] = '\0';

        /* what URL that receives this POST */
        curl_easy_setopt(curl, CURLOPT_URL, url);

        /* only disable 100-continue header if explicitly requested */
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)buff);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mem_write_callback);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);

        /* always cleanup */
        curl_easy_cleanup(curl);

        free(url);
        /* then cleanup the form */
        curl_mime_free(form);

        if(res == CURLE_OK)
            return buff;
    }

    return NULL;
}

MemStore *call_method_wp(char *token, char *method, char *params) {
    size_t len = strlen(method) + strlen(params) + 1;
    char *tmp = (char *)calloc(1, len);

    MemStore *ms = call_method(token, tmp);

    free(tmp);

    return ms;
}
